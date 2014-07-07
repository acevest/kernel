/*
 * ------------------------------------------------------------------------
 *   File Name: ide.c
 *      Author: Zhao Yanbai
 *              Sat May 24 16:30:38 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <types.h>
#include <printk.h>
#include <assert.h>
#include <io.h>
#include <ide.h>
#include <irq.h>
#include <pci.h>
#include <system.h>
#include <semaphore.h>
#include <wait.h>
#include <string.h>

typedef struct _ide_drv
{
    pci_device_t *pci;
    unsigned long pio_cnt;
    unsigned long dma_cnt;
    unsigned long irq_cnt;

    unsigned int iobase;

    unsigned int bus_cmd;
    unsigned int bus_status;
    unsigned int bus_prdt;

    unsigned int read_mode;


    u64_t   ext_lba_base;
    part_t  part[MAX_SUPPORT_PARTITION_CNT];
} ide_drive_t;

typedef struct prd
{
    unsigned int addr;
    unsigned int cnt : 16;
    unsigned int reserved : 15;
    unsigned int eot : 1;
} prd_t;

typedef struct {
    u64_t   lba;
    u32_t   scnt;
    char    *buf;
    bool    finish;
    wait_queue_head_t wait;
} ide_request_t;

typedef void (*ide_intr_func_t)();

void ide_default_intr();

ide_drive_t drv;
DECLARE_MUTEX(mutex);
ide_request_t ide_request;

ide_intr_func_t ide_intr_func = ide_default_intr;

unsigned char *dma_data = 0;

unsigned int HD_CHL0_CMD_BASE = 0x1F0;
unsigned int HD_CHL1_CMD_BASE = 0x170;

unsigned int HD_CHL0_CTL_BASE = 0x3F6;
unsigned int HD_CHL1_CTL_BASE = 0x376;

void ide_printl()
{
    printl(MPL_IDE, "ide pio_cnt %d dma_cnt %d irq_cnt %d", drv.pio_cnt,  drv.dma_cnt, drv.irq_cnt);
}

void ide_cmd_out(dev_t dev, u32 sect_cnt, u64 sect_nr, u32 cmd)
{
    drv.pio_cnt++;
    drv.read_mode = cmd;

    ide_printl();

    outb(0x00,                      REG_CTL(dev));
    outb(0x40|0x00,                 REG_DEVSEL(dev));

    outb((u8)((sect_cnt>>8)&0xFF),  REG_NSECTOR(dev));    // High
    outb((u8)((sect_nr>>24)&0xFF),  REG_LBAL(dev));
    outb((u8)((sect_nr>>32)&0xFF),  REG_LBAM(dev));
    outb((u8)((sect_nr>>40)&0xFF),  REG_LBAH(dev));

    outb((u8)((sect_cnt>>0)&0xFF),  REG_NSECTOR(dev));    // Low
    outb((u8)((sect_nr>> 0)&0xFF),  REG_LBAL(dev));
    outb((u8)((sect_nr>> 8)&0xFF),  REG_LBAM(dev));
    outb((u8)((sect_nr>>16)&0xFF),  REG_LBAH(dev));

    outb(cmd,                       REG_CMD(dev));
}

void ide_do_read(u64_t lba, u32_t scnt, char *buf)
{
    bool finish = false;
    unsigned long flags;

    ide_request_t *r = &ide_request;

    down(&mutex);

    r->lba = lba;
    r->scnt= scnt;
    r->buf = buf;
    r->finish = false;
    init_wait_queue(&r->wait);

    task_union * task = current;
    DECLARE_WAIT_QUEUE(wait, task);
    add_wait_queue(&r->wait, &wait);

    ide_cmd_out(0, scnt, lba, HD_CMD_READ_EXT);
    
    while(true)
    {
        printd("%s pid %d is going to wait\n", __func__, sysc_getpid());
        task->state = TASK_WAIT;
        irq_save(flags);
        finish = r->finish;
        irq_restore(flags);

        if(finish)
            break;

        schedule();
        printd("%s pid %d is running\n", __func__, sysc_getpid());
    }

    printd("%s pid %d is really running\n", __func__, sysc_getpid());
    task->state = TASK_RUNNING;
    del_wait_queue(&r->wait, &wait);
}


unsigned int sys_clock();

void ide_pci_init(pci_device_t *pci)
{
    unsigned int v;

    v = pci_read_config_word(pci_cmd(pci, PCI_COMMAND));
    printk(" ide pci command %04x\n", v);

    v = pci_read_config_byte(pci_cmd(pci, PCI_PROGIF));
    printk(" ide pci program interface %02x\n", v);

    unsigned int iobase = pci_read_config_long(pci_cmd(pci, PCI_BAR4));
    printk(" ide pci Base IO Address Register %08x\n", iobase);
    iobase &= 0xFFFC;
    drv.iobase      = iobase;
    drv.bus_cmd     = iobase + PCI_IDE_CMD;
    drv.bus_status  = iobase + PCI_IDE_STATUS;
    drv.bus_prdt    = iobase + PCI_IDE_PRDT;


    int i;
    printk(" BARS: ");
    for(i=0; i<6; ++i)
    {
        printk("%08x ", pci->bars[i]);
        pci->bars[i] &= (~1UL);
    }
    printk("\n");

    HD_CHL0_CMD_BASE = pci->bars[0] ? pci->bars[0] : HD_CHL0_CMD_BASE;
    HD_CHL0_CTL_BASE = pci->bars[1] ? pci->bars[1] : HD_CHL0_CTL_BASE;

    HD_CHL1_CMD_BASE = pci->bars[2] ? pci->bars[2] : HD_CHL1_CMD_BASE;
    HD_CHL1_CTL_BASE = pci->bars[3] ? pci->bars[3] : HD_CHL1_CTL_BASE;

    printk("channel0: cmd %04x ctl %04x channel1: cmd %04x ctl %04x\n", HD_CHL0_CMD_BASE, HD_CHL0_CTL_BASE, HD_CHL1_CMD_BASE, HD_CHL1_CTL_BASE);
    printl(18, "channel0: cmd %04x ctl %04x channel1: cmd %04x ctl %04x", HD_CHL0_CMD_BASE, HD_CHL0_CTL_BASE, HD_CHL1_CMD_BASE, HD_CHL1_CTL_BASE);
}


void ide_status()
{
    u8_t idest = inb(REG_STATUS(0));
    u8_t pcist = inb(drv.bus_status);
    printk(" ide status %02x pci status %02x\n", idest, pcist);
}

void ide_debug()
{
    u32    device;
    u32    nsect = 1;
    u32    retires = 100;
    u64    sect_nr = 0;
    int count=SECT_SIZE;

    nsect    = (count + SECT_SIZE -1)/SECT_SIZE;

    ide_cmd_out(0, nsect, sect_nr, HD_CMD_READ_EXT);
}


void init_pci_controller(unsigned int classcode)
{
    pci_device_t *pci = pci_find_device_by_classcode(classcode);
    if(pci != 0 && pci->intr_line < 16)
    {
        printk("Found PCI Vendor %04x Device %04x Class %04x IntrLine %d\n", pci->vendor, pci->device, pci->classcode, pci->intr_line);
        printl(17, "Found PCI Vendor %04x Device %04x Class %04x IntrLine %d", pci->vendor, pci->device, pci->classcode, pci->intr_line);
        ide_pci_init(pci);
        drv.pci = pci;
    }
}


void ide_default_intr()
{
    u8_t status = inb(REG_STATUS(0));

    drv.irq_cnt++;

    status = inb(drv.bus_status);
    if(0 == (status & PCI_IDE_STATUS_INTR))
    {
        return ;
    }

    status |= PCI_IDE_STATUS_INTR;
    outb(status, drv.bus_status);
    outb(0x00,   drv.bus_cmd);

    u16_t sig = 0;
    if(drv.read_mode == HD_CMD_READ_EXT)
    {
        insl(REG_DATA(0), ide_request.buf, ((ide_request.scnt*SECT_SIZE)>>2));
        sig = *((u16_t *) (ide_request.buf+510));
    }

    if(drv.read_mode == HD_CMD_READ_DMA)
    {
        sig = *((u16_t *) (dma_data+510));
    }

    ide_printl();

    printd(" hard disk sig %04x read mode %x cnt %d\n", sig, drv.read_mode, drv.irq_cnt);
    printl(MPL_IDE_INTR, "hard disk sig %x read mode %x cnt %d", sig, drv.read_mode, drv.irq_cnt);

    outb(PCI_IDE_CMD_STOP, drv.bus_cmd);

    wake_up(&ide_request.wait);
    ide_request.finish = true;

    up(&mutex);
}


void ide_irq()
{
    ide_intr_func();
}

prd_t prd __attribute__((aligned(64*1024)));
unsigned long gprdt = 0;

#define DELAY400NS {        \
    inb(HD_CHL0_CTL_BASE);  \
    inb(HD_CHL0_CTL_BASE);  \
    inb(HD_CHL0_CTL_BASE);  \
    inb(HD_CHL0_CTL_BASE);  \
}

void ide_dma_pci_lba48()
{
    drv.dma_cnt ++;
    drv.read_mode = HD_CMD_READ_DMA;
#if 1
    memset((void *)&prd, 0, sizeof(prd));
    unsigned long addr = alloc_one_page(0);
    dma_data = (char *) addr;
    memset(dma_data, 0xBB, 512);
    prd.addr = va2pa(addr);
    prd.cnt  = 512;
    prd.eot  = 1;
    gprdt = va2pa(&prd);

    printl(16, "gprdt %08x &prdt %08x prd.addr %08x addr %08x",
            gprdt, &prd, prd.addr, addr);

    outb(PCI_IDE_CMD_STOP, drv.bus_cmd);
    unsigned short status = inb(drv.bus_status);
    outb(status | PCI_IDE_STATUS_INTR | PCI_IDE_STATUS_ERR, drv.bus_status);
    outl(gprdt, drv.bus_prdt);
    outb(PCI_IDE_CMD_WRITE, drv.bus_cmd);
#endif

#if 0
    while ( 1 )
    {
        status = inb(HD_CHL0_CMD_BASE+HD_STATUS);
        printk(" <%02x> ", status);
        if((status & (HD_STATUS_BSY | HD_STATUS_DRQ)) == 0)
        {
            break;
        }
    }

    outb(0x00, HD_CHL0_CMD_BASE+HD_DEVSEL);
    DELAY400NS;

    while ( 1 )
    {
        status = inb(HD_CHL0_CMD_BASE+HD_STATUS);
        printk(" <%02x> ", status);
        if((status & (HD_STATUS_BSY | HD_STATUS_DRQ)) == 0)
        {
            break;
        }
    }
#endif

    outb(0x00,  HD_CHL0_CTL_BASE);  // Device Control

    outb(0x00,  HD_CHL0_CMD_BASE+HD_FEATURES);
    outb(0x00,  HD_CHL0_CMD_BASE+HD_NSECTOR);
    outb(0x00,  HD_CHL0_CMD_BASE+HD_LBAL);
    outb(0x00,  HD_CHL0_CMD_BASE+HD_LBAM);
    outb(0x00,  HD_CHL0_CMD_BASE+HD_LBAH);

    outb(0x00,  HD_CHL0_CMD_BASE+HD_FEATURES);
    outb(0x01,  HD_CHL0_CMD_BASE+HD_NSECTOR);
    outb(0x00,  HD_CHL0_CMD_BASE+HD_LBAL);
    outb(0x00,  HD_CHL0_CMD_BASE+HD_LBAM);
    outb(0x00,  HD_CHL0_CMD_BASE+HD_LBAH);

    outb(0x40,  HD_CHL0_CMD_BASE+HD_DEVSEL);

    outb(HD_CMD_READ_DMA,  HD_CHL0_CMD_BASE+HD_CMD);

    inb(drv.bus_cmd);
    inb(drv.bus_status);
    unsigned short w = inb(drv.bus_cmd);
    outb(w|PCI_IDE_CMD_WRITE|PCI_IDE_CMD_START, drv.bus_cmd);
    inb(drv.bus_cmd);
    inb(drv.bus_status);
}

typedef struct {
    u8_t a;
    u8_t b;
    u16_t lbah;  // lba high
    u8_t type;
    u8_t f;
    u16_t scnth; // sector count high
    u32_t lba;   // lba low
    u32_t scnt;  // sector count
} hd_part_t ;


void ide_read_extended_partition(u64_t lba, unsigned int inx)
{
    if(inx >= MAX_SUPPORT_PARTITION_CNT)
        return ;

    unsigned int i;
    char *buf = kmalloc(512, 0);
    if(buf == 0)
        panic("no memory");

    ide_do_read(lba, 1, buf);

    u16_t sig = *((u16_t *) (buf+510));
    if(sig != 0xAA55)
        panic("bad partition sect");

    hd_part_t *p = (hd_part_t *)(buf+PARTITION_TABLE_OFFSET);
    printd("%s:%d lba %d \n", __func__, __LINE__, lba);

    for(i=0; i<PARTITION_CNT; ++i, ++p)
    {
        if(p->type == 0)
            continue;

        //u64_t   part_lba = lba + (p->lba|((p->lbah*1ULL)<<32));
        //u64_t   part_scnt= p->scnt | ((p->scnth*1ULL)<<32);
        u64_t   part_lba = lba + p->lba;
        u64_t   part_scnt   = p->scnt;

        if(p->type != 0x05)
        {
            drv.part[inx].lba_start  = part_lba;
            drv.part[inx].lba_end    = part_lba+part_scnt;
            printk("  Logic Partition[%02d] [%02x] LbaBase %10d LbaEnd %10d\n", inx, p->type, (unsigned int)(drv.part[inx].lba_start), (unsigned int)(drv.part[inx].lba_end - 1));
        }
        else
        {
            part_lba = drv.ext_lba_base + p->lba;
            printk("        Extended      [%02x] LbaBase %10d LbaEnd %10d\n", p->type, (unsigned int)(part_lba), (unsigned int)(part_lba+part_scnt - 1));
            ide_read_extended_partition(part_lba, inx+1);
        }
    }

    kfree(buf);
}

void ide_read_partition()
{
    printk("Reading Partitions....\n");
    unsigned int i;
    char *buf = kmalloc(512, 0);
    if(buf == 0)
        panic("no memory");

    ide_do_read(0, 1, buf);

    u16_t sig = *((u16_t *) (buf+510));
    if(sig != 0xAA55)
        panic("bad partition sect");

    hd_part_t *p = (hd_part_t *)(buf+PARTITION_TABLE_OFFSET);

    unsigned int ext_inx = ~0U;

    for(i=0; i<PARTITION_CNT; ++i, ++p)
    {
        if(p->type == 0)
            continue;

        //u64_t   part_lba = p->lba|((p->lbah*1ULL)<<32);
        //u64_t   part_scnt= p->scnt | ((p->scnth*1ULL)<<32);
        u64_t   part_lba    = p->lba;
        u64_t   part_scnt   = p->scnt;

        drv.part[i].lba_start  = part_lba;
        drv.part[i].lba_end    = part_lba+part_scnt;


        if(p->type == 0x05)
        {
            if(drv.ext_lba_base == 0)
            {
                drv.ext_lba_base = drv.part[i].lba_start; 
                ext_inx = i;
            }
        }

        printk("Primary Partition[%02d] [%02x] LbaBase %10d LbaEnd %10d\n", i, p->type, (unsigned int)(part_lba), (unsigned int)(part_lba+part_scnt - 1));
    }

    kfree(buf);

    if(ext_inx != ~0U)
        ide_read_extended_partition(drv.part[ext_inx].lba_start, 4);
}

void ide_init()
{
    memset((void *)&drv, 0, sizeof(drv));

    init_pci_controller(0x0106);
    init_pci_controller(0x0101);

    ide_read_partition();

    ide_printl();
}

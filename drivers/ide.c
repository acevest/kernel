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

unsigned int HD_CHL0_CMD_BASE = 0x1F0;
unsigned int HD_CHL1_CMD_BASE = 0x170;

unsigned int HD_CHL0_CTL_BASE = 0x3F6;
unsigned int HD_CHL1_CTL_BASE = 0x376;

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

ide_drive_t drv;

typedef struct prd
{
    unsigned int addr;
    unsigned int cnt : 16;
    unsigned int reserved : 15;
    unsigned int eot : 1;
} prd_t;

unsigned char *data = 0;

unsigned int sys_clock();

bool ide_init_inted = false;

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
    printd(18, "channel0: cmd %04x ctl %04x channel1: cmd %04x ctl %04x", HD_CHL0_CMD_BASE, HD_CHL0_CTL_BASE, HD_CHL1_CMD_BASE, HD_CHL1_CTL_BASE);
}


void ide_printd()
{
    printd(MPL_IDE, "ide pio_cnt %d dma_cnt %d irq_cnt %d", drv.pio_cnt,  drv.dma_cnt, drv.irq_cnt);
}

void _ide_cmd_out(dev_t dev, u32 sect_cnt, u64 sect_nr, u32 cmd, bool pio)
{
    ide_init_inted = false;

    drv.pio_cnt++;
    drv.read_mode = cmd;

    ide_printd();

    outb(0x00|(pio?0:HD_CTL_NIEN),  REG_CTL(dev));
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

void ide_wait_ready()
{
    u32 retires = 100;
    
    do
    {
        int drq_retires = 100000;
        while(!hd_drq(dev) && --drq_retires)
            /* do nothing */;

        if(drq_retires != 0)
            break;
    }while(--retires);

    if(retires == 0)
        panic("hard disk is not ready");
}


void ide_cmd_out(dev_t dev, u32 sect_cnt, u64 sect_nr, u32 cmd)
{
    _ide_cmd_out(dev, sect_cnt, sect_nr, cmd, true);
}

void dump_pci_ide();

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
    u32    sect_nr = 0;
    int count=SECT_SIZE;

    nsect    = (count + SECT_SIZE -1)/SECT_SIZE;

    ide_cmd_out(0, nsect, sect_nr, HD_CMD_READ_EXT);
}

DECLARE_MUTEX(mutex);

void init_pci_controller(unsigned int classcode)
{
    pci_device_t *pci = pci_find_device_by_classcode(classcode);
    if(pci != 0 && pci->intr_line < 16)
    {
        printk("Found PCI Vendor %04x Device %04x Class %04x IntrLine %d\n", pci->vendor, pci->device, pci->classcode, pci->intr_line);
        printd(17, "Found PCI Vendor %04x Device %04x Class %04x IntrLine %d", pci->vendor, pci->device, pci->classcode, pci->intr_line);
        ide_pci_init(pci);
        drv.pci = pci;
    }
}


char ide_buf[1024];
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
        insl(REG_DATA(0), ide_buf, (512>>2));
        sig = *((u16_t *) (ide_buf+510));
    }

    if(drv.read_mode == HD_CMD_READ_DMA)
    {
        sig = *((u16_t *) (data+510));
    }

    ide_printd();

    printk("hard disk sig %04x read mode %x cnt %d\n", sig, drv.read_mode, drv.irq_cnt);
    printd(MPL_IDE_INTR, "hard disk sig %x read mode %x cnt %d", sig, drv.read_mode, drv.irq_cnt);

    outb(PCI_IDE_CMD_STOP, drv.bus_cmd);
    up(&mutex);
}

typedef void (*ide_intr_func_t)();
ide_intr_func_t ide_intr_func = ide_default_intr;

void ide_irq()
{
    ide_intr_func();
}

void print_ide_identify(const char *buf)
{
    char *p;
    short *ident;
    int i, j;
    unsigned char c;

    ident = (short *) buf;

    char    hd_sn[32];    /* 20 bytes */
    char    hd_model[64];    /* 40 bytes */
    short   hd_capabilites = ident[49];
    short   hd_supt_inst_set = ident[83];

    p = (char *) (ident+10);
    for(i=0; i<20; i++)
        hd_sn[i] = p[i];
    for(j=0; j<20; j+=2)
    {
        c = hd_sn[j];
        hd_sn[j] = hd_sn[j+1];
        hd_sn[j+1] = c;
    }

    hd_sn[i] = 0;

    p = (char *) (ident+27);
    for(i=0; i<40; i++)
    {
        hd_model[i] = p[i];
    }
    for(j=0; j<20; j+=2)
    {
        c = hd_model[j];
        hd_model[j] = hd_model[j+1];
        hd_model[j+1] = c;
    }
    hd_model[i] = 0;


    printk("Hard Disk Vendor: %s\n", hd_sn);
    printk("Hard Disk Model: %s\n", hd_model);
    printk("Hard Disk Support LBA: %s\n",
        (hd_capabilites & 0x0200) ? "Yes" : "No");
    printk("Hard Disk Support LBA-48bit: %s\n",
        (hd_supt_inst_set & 0x0400) ? "Yes" : "No");

    if(!(hd_supt_inst_set & 0x0400))
        panic("Your hard disk ");
}

void ide_read_identify()
{
    outb(HD_CTL_NIEN,               REG_CTL(dev));
    outb(0x00,                      REG_DEVSEL(dev));
    outb(HD_CMD_IDENTIFY,           REG_CMD(dev));

    ide_wait_ready();

    insl(REG_DATA(0), ide_buf, 512>>2);
    print_ide_identify(ide_buf);
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
    ide_init_inted = false;
    drv.dma_cnt ++;
    drv.read_mode = HD_CMD_READ_DMA;
#if 1
    memset((void *)&prd, 0, sizeof(prd));
    unsigned long addr = alloc_one_page(0);
    data = (char *) addr;
    memset(data, 0xBB, 512);
    prd.addr = va2pa(addr);
    prd.cnt  = 512;
    prd.eot  = 1;
    gprdt = va2pa(&prd);

    printd(16, "gprdt %08x &prdt %08x prd.addr %08x addr %08x",
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
    unsigned char a;
    unsigned char b;
    unsigned char c;
    unsigned char d;
    unsigned char type;
    unsigned char f;
    unsigned char g;
    unsigned char h;
    unsigned int lba;
    unsigned int sect_cnt;
} hd_part_t ;



char *ide_init_buf = 0;
void ide_init_intr()
{
    drv.irq_cnt++;

    u8_t status = inb(REG_STATUS(0));

    insl(REG_DATA(0), ide_init_buf, (512>>2));

    ide_init_inted = true;
}

void ide_init_wait_intr()
{
    unsigned int clock_end = sys_clock() + 1000;

    while(!ide_init_inted)
    {
        if(sys_clock() > clock_end)
            panic("read hard disk timeout");
    }
}

void ide_init_wait_read(u64 lba, char *buf)
{
    ide_init_buf = buf;
    _ide_cmd_out(0, 1, lba, HD_CMD_READ_EXT, true);
    ide_init_wait_intr();
}

#if 0
void ide_read_partition(u64_t lba, bool extended)
{
    unsigned int i;
    char *buf = kmalloc(512, 0);
    if(buf == 0)
        panic("no memory");

    ide_init_wait_read(lba, buf);

    u16_t sig = *((u16_t *) (buf+510));
    if(sig != 0xAA55)
        panic("bad partition sect");

    hd_part_t *p = (hd_part_t *)(buf+PARTITION_TABLE_OFFSET);
    printk("-------------------------\n");
    for(i=0; i<PARTITION_CNT; ++i, ++p)
    {
        if(p->type == 0)
            continue;

        u64_t   part_lba    = p->lba;       // TODO
        u64_t   part_scnt   = p->sect_cnt;

        if(p->type == 0x05)
            printk(" Partition[%d] [%02x] LBA %d %d\n", i, p->type, (unsigned int)part_lba, (unsigned int)((extended?drv.ext_lba_base:0)+part_lba));
        else
            printk(" Partition[%d] [%02x] LBA %d %d\n", i, p->type, (unsigned int)part_lba, (unsigned int)(lba+part_lba));

        if(p->type == 0x05)
        {
            if(drv.ext_lba_base == 0)
                drv.ext_lba_base =part_lba; 


            ide_read_partition(part_lba+(extended?drv.ext_lba_base:0), true);
        }
    }

    kfree(buf);
}
#endif

void ide_read_extended_partition(u64_t lba, unsigned int inx)
{
    unsigned int i;
    char *buf = kmalloc(512, 0);
    if(buf == 0)
        panic("no memory");

    ide_init_wait_read(lba, buf);

    u16_t sig = *((u16_t *) (buf+510));
    if(sig != 0xAA55)
        panic("bad partition sect");

    hd_part_t *p = (hd_part_t *)(buf+PARTITION_TABLE_OFFSET);
    //printk("-------------------------%d \n", lba);

    for(i=0; i<PARTITION_CNT; ++i, ++p)
    {
        if(p->type == 0)
            continue;

        u64_t   part_lba = lba + p->lba;       // TODO
        u64_t   part_scnt   = p->sect_cnt;

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

    ide_init_wait_read(0, buf);

    u16_t sig = *((u16_t *) (buf+510));
    if(sig != 0xAA55)
        panic("bad partition sect");

    hd_part_t *p = (hd_part_t *)(buf+PARTITION_TABLE_OFFSET);

    unsigned int ext_inx = ~0U;

    for(i=1; i<PARTITION_CNT; ++i, ++p)
    {
        if(p->type == 0)
            continue;

        u64_t   part_lba    = p->lba;       // TODO
        u64_t   part_scnt   = p->sect_cnt;

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
        ide_read_extended_partition(drv.part[ext_inx].lba_start, 5);
}



void ide_init_partition()
{
    ide_intr_func = ide_init_intr;

    ide_read_partition();

    ide_intr_func = ide_default_intr;
}

void ide_init()
{
    memset((void *)&drv, 0, sizeof(drv));
    init_pci_controller(0x0106);
    init_pci_controller(0x0101);

#if 1
    ide_read_identify();
    ide_printd();
#endif

    ide_init_partition();
}

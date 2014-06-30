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
    unsigned long cmd_cnt;
    unsigned long irq_cnt;

    unsigned int iobase;

    unsigned int bus_cmd;
    unsigned int bus_status;
    unsigned int bus_prdt;
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
    printd(MPL_IDE, "ide cmd cnt %d irq cnt %d", drv.cmd_cnt,  drv.irq_cnt);
}
void ide_cmd_out(Dev dev, u32 nsect, u64 sect_nr, u32 cmd)
{
    drv.cmd_cnt++;

    outb(0x00,                      REG_CTL(dev));
    outb(0x40,                      REG_DEVSEL(dev));

    outb(0,                         REG_NSECTOR(dev));    // High
    outb((u8)((sect_nr>>24)&0xFF),  REG_LBAL(dev));
    outb((u8)((sect_nr>>32)&0xFF),  REG_LBAM(dev));
    outb((u8)((sect_nr>>40)&0xFF),  REG_LBAH(dev));

    outb((u8)nsect,                 REG_NSECTOR(dev));    // Low
    outb((u8)((sect_nr>> 0)&0xFF),  REG_LBAL(dev));
    outb((u8)((sect_nr>> 8)&0xFF),  REG_LBAM(dev));
    outb((u8)((sect_nr>>16)&0xFF),  REG_LBAH(dev));

    outb(cmd,                       REG_CMD(dev));

    ide_printd();
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

    ide_cmd_out(0, nsect,  sect_nr, HD_CMD_READ_EXT);
}

DECLARE_MUTEX(mutex);

void init_pci_controller(unsigned int vendor, unsigned int device)
{
    pci_device_t *pci = pci_find_device(vendor, device);
    if(pci != 0)
    {
        printk("Found PCI Vendor %04x Device %04x Class %04x IntrLine %d\n", vendor, device, pci->classcode, pci->intr_line);
        printd(17, "Found PCI Vendor %04x Device %04x Class %04x IntrLine %d", vendor, device, pci->classcode, pci->intr_line);
        ide_pci_init(pci);
        drv.pci = pci;
    }
}

char buf[1024];
void ide_irq()
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

    insl(REG_DATA(0), buf, (512>>2));
    u16_t sig = *((u16_t *) (buf+510));
    printk("hard disk data %04x\n", sig);
    sig = *((u16_t *) (data+510));
    printk("hard disk data %04x\n", sig);
    ide_printd();

    up(&mutex);
}


void print_ide_identify(const char *buf)
{
    char    *p;
    short    *ident;
    int    i;

    ident = (short *) buf;

    char    hd_sn[32];    /* 20 bytes */
    char    hd_model[64];    /* 40 bytes */
    short   hd_capabilites = ident[49];
    short   hd_supt_inst_set = ident[83];

    p = (char *) (ident+10);
    for(i=0; i<20; i++)
    {
        hd_sn[i] = p[i];
    }
    hd_sn[i] = 0;

    p = (char *) (ident+27);
    for(i=0; i<40; i++)
    {
        hd_model[i] = p[i];
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
    outb(0x02, REG_CTL(0));
    outb(0x40,                      REG_DEVSEL(dev));
    outb(HD_CMD_IDENTIFY,           REG_CMD(dev));

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

    insl(REG_DATA(0), buf, 512>>2);
    print_ide_identify(buf);
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

#if 0
    pci_write_config_word(PCI_IDE_CMD_STOP, drv.bus_cmd);
    unsigned char status;
    status = pci_read_config_long(drv.bus_status);
    pci_write_config_word(status | PCI_IDE_STATUS_INTR | PCI_IDE_STATUS_ERR, drv.bus_status);
    pci_write_config_long(gprdt, drv.bus_prdt);
    pci_write_config_word(PCI_IDE_CMD_WRITE, drv.bus_cmd);
#else
    outb(PCI_IDE_CMD_STOP, drv.bus_cmd);
    unsigned short status = inb(drv.bus_status);
    outb(status | PCI_IDE_STATUS_INTR | PCI_IDE_STATUS_ERR, drv.bus_status);
    outl(gprdt, drv.bus_prdt);
    outb(PCI_IDE_CMD_WRITE, drv.bus_cmd);
#endif
#endif

#if 1
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
    outb(0x00, HD_CHL0_CMD_BASE+HD_DEVSEL);
    DELAY400NS;

#if 1
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

    printd(16, "---------------------------------------");

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

    outb(0x25, HD_CHL0_CMD_BASE+HD_CMD);

#if 0
    pci_read_config_word(drv.bus_cmd);
    pci_read_config_word(drv.bus_status);
    pci_write_config_word(PCI_IDE_CMD_WRITE|PCI_IDE_CMD_START, drv.bus_cmd);
    pci_read_config_word(drv.bus_cmd);
    pci_read_config_word(drv.bus_status);
#else
    inb(drv.bus_cmd);
    inb(drv.bus_status);
    unsigned short w = inb(drv.bus_cmd);
    outb(w|PCI_IDE_CMD_WRITE|PCI_IDE_CMD_START, drv.bus_cmd);
    inb(drv.bus_cmd);
    inb(drv.bus_status);
#endif
}

void ide_init()
{
    memset((void *)&drv, 0, sizeof(drv));
    init_pci_controller(PCI_VENDORID_INTEL, 0x2829);
    init_pci_controller(PCI_VENDORID_INTEL, 0x7010);
#if 0
    ide_read_identify();
    ide_printd();
#endif
}

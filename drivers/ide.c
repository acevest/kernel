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

unsigned int HD_CHL0_CMD_BASE = 0x1F0;
unsigned int HD_CHL1_CMD_BASE = 0x170;


unsigned int HD_CHL0_CTL_BASE = 0x3F6;
unsigned int HD_CHL1_CTL_BASE = 0x376;

typedef struct _hd_drv
{
    pci_device_t *pci;
} hd_drive_t;

hd_drive_t hdrv;

unsigned int iobase;
typedef struct prd
{
    unsigned int addr;
    unsigned int cnt : 16;
    unsigned int reserved : 15;
    unsigned int eot : 1;
} prd_t;

#define PRD_CNT 1
#define USE_DMA 0
prd_t hd_prd_tbl[PRD_CNT] __attribute__((aligned(64*1024)));
unsigned long prdt_phys = 0;
void ide_pci_init(pci_device_t *pci)
{
    unsigned int v;
    unsigned int progif;
    v = pci_read_config_word(pci_cmd(pci, PCI_COMMAND));
    printk(" ide pci command %04x\n", v);
    v = pci_read_config_byte(pci_cmd(pci, PCI_PROGIF));
    progif = v & 0xFF;
    printk(" ide pci program interface %02x\n", v);
    v = pci_read_config_long(pci_cmd(pci, PCI_BAR4));
    printk(" ide pci Base IO Address Register %08x\n", v);
    iobase = v & 0xFFFC;

#if USE_DMA
    v = inb(iobase+0);
    printk(" ide bus master ide command register primary %04x\n", v);
    v = inb(iobase+2);
    printk(" ide bus master ide status register primary %04x\n", v);
#endif

    int i;
    printk(" BARS: ");
    for(i=0; i<6; ++i)
    {
        printk("%08x ", pci->bars[i]);
        pci->bars[i] &= (~1UL);
    }
    printk("\n");

#if USE_DMA
    prd_t *p = (prd_t *) va2pa(hd_prd_tbl);
    printk("iobase %04x hd_prd_tbl %08x physical %08x sizeof prd %d\n", iobase, hd_prd_tbl, p, sizeof(prd_t));
    p->addr = 0;
    p->cnt  = 512;
    p->reserved = 0;
    p->eot = 1;
    prdt_phys = (unsigned long) p;
    printk(" ide bus master ide status register primary %04x\n", v);
#endif

    HD_CHL0_CMD_BASE = pci->bars[0] ? pci->bars[0] : HD_CHL0_CMD_BASE;
    HD_CHL0_CTL_BASE = pci->bars[1] ? pci->bars[1] : HD_CHL0_CTL_BASE;

    HD_CHL1_CMD_BASE = pci->bars[2] ? pci->bars[2] : HD_CHL1_CMD_BASE;
    HD_CHL1_CTL_BASE = pci->bars[3] ? pci->bars[3] : HD_CHL1_CTL_BASE;

    printk("channel0: cmd %04x ctl %04x channel1: cmd %04x ctl %04x\n", HD_CHL0_CMD_BASE, HD_CHL0_CTL_BASE, HD_CHL1_CMD_BASE, HD_CHL1_CTL_BASE);
}

void ide_hd_out(Dev dev, u32 nsect, u64 sect_nr, u32 cmd)
{
#if USE_DMA
    outb(0x00, iobase);

    //outl(prdt_phys, iobase+4);
#if 1
    outb((prdt_phys)&0xFF, iobase+0);
    outb((prdt_phys>>8)&0xFF, iobase+1);
    outb((prdt_phys>>16)&0xFF, iobase+2);
    outb((prdt_phys>>24)&0xFF, iobase+3);
#endif

    outb(0x09,    iobase);
#endif

    {
    unsigned long long sect_nr = 0;
    unsigned int nsect = 1;

    outb(0x00,      REG_CTL(dev));
    outb(0x40,      REG_DEVSEL(dev));

    outb(0,           REG_NSECTOR(dev));    // High
    outb((u8)((sect_nr>>24)&0xFF),    REG_LBAL(dev));
    outb((u8)((sect_nr>>32)&0xFF),    REG_LBAM(dev));
    outb((u8)((sect_nr>>40)&0xFF),    REG_LBAH(dev));

    outb((u8)nsect,   REG_NSECTOR(dev));    // Low
    outb((u8)((sect_nr>> 0)&0xFF),    REG_LBAL(dev));
    outb((u8)((sect_nr>> 8)&0xFF),    REG_LBAM(dev));
    outb((u8)((sect_nr>>16)&0xFF),    REG_LBAH(dev));

    outb(0x24,    REG_CMD(dev));
    }
}


void dump_pci_ide();

void ide_status()
{
    u8_t idest = inb(REG_STATUS(0));
    u8_t pcist = inb(iobase+2);
    printk(" ide status %02x pci status %02x\n", idest, pcist);

}
void ide_debug()
{
    u32    device;
    u32    nsect = 1;
    u32    retires = 100;
    u32    sect_nr = 1;
    int count=SECT_SIZE;

    nsect    = (count + SECT_SIZE -1)/SECT_SIZE;

    dump_pci_ide();

    ide_hd_out(0, nsect,  sect_nr, HD_CMD_READ_DMA);

    printk("ide_debug\n");
}

void dump_pci_controller(unsigned int vendor, unsigned int device)
{
    pci_device_t *pci = pci_find_device(vendor, device);
    if(pci != 0)
    {
        printk("Found PCI Vendor %04x Device %04x Class %04x IntrLine %d\n", vendor, device, pci->classcode, pci->intr_line);
        ide_pci_init(pci);
        hdrv.pci = pci;
    }
}

void dump_pci_ide()
{
    dump_pci_controller(PCI_VENDORID_INTEL, 0x2922);
    dump_pci_controller(PCI_VENDORID_INTEL, 0x2829);
    //dump_pci_controller(PCI_VENDORID_INTEL, 0x7000);
}

void ide_irq()
{
    u8_t sa = inb(REG_STATUS(0));
#if 1
    unsigned short *p = (unsigned short *) (0+510);
    printk("||----------------- %s:%d  status %04x SIG: %04x\n", __func__, __LINE__, sa, *p);
    u8_t v = inb(iobase+2);
    printk(" irq pci ide status register primary %02x\n", v);
    v |= 0x04;
    printk(" irq pci ide status before write %02x\n", v);
    //outb(v, iobase+2);
    v = inb(iobase+2);
    printk(" irq pci ide status after write %02x\n", v);
    outb(0x00, iobase);

    char buf[1024];
    memset(buf, 0xEE, 1024);
    insw(REG_DATA(0), buf, 512>>1);
    unsigned short *s = (unsigned short *) (buf+510);
    printk("insw %04x\n", *s);
#else
    char buf[1024];
    memset(buf, 0xEE, 1024);
    //hd_rd_data(0, buf, 512);
    insw(REG_DATA(0), buf, 512>>1);
    unsigned short *p = (unsigned short *) (buf+510);
    unsigned short *s = (unsigned short *) (buf+512);
    u8_t sb = inb(REG_STATUS(0));
    printk("sata %04x sa %02x sb %02x REG_STATUS %x REG_DATA %x SIG %02x\n", *p, sa, sb, REG_STATUS(0), REG_DATA(0), *s);
    dump_pci_ide();

    unsigned short ctl = inw(iobase+0);
    printk(" ide bus master ide command register primary %04x\n", ctl);
    unsigned short sts = inw(iobase+2);
    printk(" ide bus master ide status register primary %04x\n", sts);
    outw(0x04, iobase+0);
    outw(0x00, iobase);
    outw(0x00, iobase+2);
#endif
}

void ide_init()
{
    pci_device_t *pci = 0;
    //dump_pci_ide();
#if 0
    pci = pci_find_device(PCI_VENDORID_INTEL, 0x7010); // qemu
    if(pci == 0)
    {
        printk("qemu ide controller\n");
    }
#endif
#if 0
    pci_device_t *pci = pci_find_device(PCI_VENDORID_INTEL, 0x2922);
    if(pci != 0)
        printk("laptop ide....\n");
#endif
#if 0
    pci_device_t *pci = pci_find_device(PCI_VENDORID_INTEL, 0x2829);
    if(pci != 0)
        printk("laptop achi....\n");
#endif
#if 0
    pci_device_t *pci = pci_find_device(PCI_VENDORID_INTEL, 0x2850);
    if(pci != 0)
        printk("laptop ide....\n");
#endif
    
#if 0
    if(pci == 0)
        panic("can not find ide device");

    printk("found ide pci device\n");
    ide_pci_init(pci);
#endif

#if 0
    outb_p(0x02, REG_CTL(0));
    ide_read_identify();
#endif

}

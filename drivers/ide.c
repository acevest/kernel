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
#include <hd.h>
#include <irq.h>
#include <pci.h>
#include <system.h>

/* ata command base register */
#define HD_CHL0_CMD_BASE    0x1F0
#define HD_CHL1_CMD_BASE    0x170

#define HD_DATA         0
#define HD_FEATURES     1
#define HD_ERR          1
#define     HD_ERR_BB        0x80
#define     HD_ERR_ECC       0x40
#define     HD_ERR_ID        0x10
#define     HD_ERR_AC        0x04
#define     HD_ERR_TK        0x02
#define     HD_ERR_DM        0x01
#define HD_NSECTOR      2
#define HD_LBAL         3
#define HD_LBAM         4
#define HD_LBAH         5
#define HD_DEVSEL       6
#define HD_CMD          7
#define HD_STATUS       7        /* controller status */
#define     HD_STATUS_BSY       0x80    /* controller busy */
#define     HD_STATUS_RDY       0x40    /* drive ready */
#define     HD_STATUS_WF        0x20    /* write fault */
#define     HD_STATUS_SEEK_CMPT 0x10    /* seek complete */
#define     HD_STATUS_DRQ       0x08    /* data transfer request */
#define     HD_STATUS_CRD       0x04    /* correct data */
#define     HD_STATUS_IDX       0x02    /* index pulse */
#define     HD_STATUS_ERR       0x01    /* error */
#define     HD_CMD_IDLE         0x00
#define     HD_CMD_RECALIBRATE  0x10
#define     HD_CMD_READ         0x20    /* read data */
#define     HD_CMD_READ_EXT     0x24    /* read data (LBA-48 bit)*/
#define     HD_CMD_READ_DMA     0x25    /* read data DMA LBA48 */
#define     HD_CMD_WRITE        0x30
#define     HD_CMD_WRITE_EXT    0x34
#define     HD_CMD_READ_VERIFY  0x40
#define     HD_CMD_FORMAT       0x50
#define     HD_CMD_SEEK         0x70
#define     HD_CMD_DIAG         0x90
#define     HD_CMD_SPECIFY      0x91
#define     HD_CMD_IDENTIFY     0xEC

/* hard disk control register */
#define HD_CHL0_CTL_BASE    0x3F6
#define HD_CHL1_CTL_BASE    0x376


#define HD_CTL            0
#define     HD_CTL_NORETRY      0x80    /* disable access retry */
#define     HD_CTL_NOECC        0x40    /* disable ecc retry */
#define     HD_CTL_EIGHTHEADS   0x08    /* more than 8 heads */
#define     HD_CTL_RESET        0x04    /* reset controller */
#define     HD_CTL_DISABLE_INT  0x02    /* disable interrupts */

#define     HD_GET_CHL(dev)     (0)     /* only support channel 0 */
#define     HD_GET_DEV(dev)     (0)     /* only support one hard disk */

#define REG_CMD_BASE(dev, offset)  ( HD_GET_CHL(dev) ? (HD_CHL1_CMD_BASE+offset) : (HD_CHL0_CMD_BASE+offset) )
#define REG_CTL_BASE(dev, offset)  ( HD_GET_CHL(dev) ? (HD_CHL1_CTL_BASE+offset) : (HD_CHL0_CTL_BASE+offset) )

#define REG_DATA(dev)       REG_CMD_BASE(dev, HD_DATA)
#define REG_ERR(dev)        REG_CMD_BASE(dev, HD_ERR)
#define REG_NSECTOR(dev)    REG_CMD_BASE(dev, HD_NSECTOR)
#define REG_LBAL(dev)       REG_CMD_BASE(dev, HD_LBAL)
#define REG_LBAM(dev)       REG_CMD_BASE(dev, HD_LBAM)
#define REG_LBAH(dev)       REG_CMD_BASE(dev, HD_LBAH)
#define REG_DEVSEL(dev)     REG_CMD_BASE(dev, HD_DEVSEL)
#define REG_STATUS(dev)     REG_CMD_BASE(dev, HD_STATUS)
#define REG_FEATURES(dev)   REG_CMD_BASE(dev, HD_FEATURES)
#define REG_CMD(dev)        REG_CMD_BASE(dev, HD_CMD)
#define REG_CTL(dev)        REG_CTL_BASE(dev, HD_CTL)

#define SECT_SIZE    512


#define hd_rd_data(dev, buf, count) hd_rd_port(REG_DATA(dev), buf, count)

#define hd_bsy(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_BSY))
#define hd_rdy(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_RDY))
#define hd_drq(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_DRQ))
#define hd_err(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_ERR))



#define ATA_IDENT_DEVTYPE               0
#define ATA_IDENT_CYLINDERS             2
#define ATA_IDENT_HEADS                 6
#define ATA_IDENT_SECTORS               12
#define ATA_IDENT_SERIAL                20
#define ATA_IDENT_MODEL                 54
#define ATA_IDENT_CAPABILITIES          98
#define ATA_IDENT_FIELDVALID            106
#define ATA_IDENT_MAX_LBA               120
#define ATA_IDENT_COMMANDSETS           164
#define ATA_IDENT_MAX_LBA_EXT           200

void ide_read_identify()
{
    unsigned char *buf = (unsigned char *) alloc_one_page(0);

    // Select Drive
    outb_p(0xA0,  REG_DEVSEL(0));

    int d;
    d = 10000000;
    while(d--);

    outb(0,   REG_NSECTOR(0));    // High
    d = 10000000;
    while(d--);
    outb(1,   REG_NSECTOR(0));    // Low

    d = 10000000;
    while(d--);
    outb(0,    REG_LBAL(0));
    d = 10000000;
    while(d--);
    outb(1,    REG_LBAL(0));

    d = 10000000;
    while(d--);
    outb(0,    REG_LBAM(0));
    d = 10000000;
    while(d--);
    outb(0,    REG_LBAM(0));

    d = 10000000;
    while(d--);
    outb(0,    REG_LBAH(0));
    d = 10000000;
    while(d--);
    outb(0,    REG_LBAH(0));

    d = 10000000;
    while(d--);
    outb_p(HD_CMD_IDENTIFY,   REG_CMD(0));

    d = 10000000;
    while(d--);

    while(1) {
        u8_t status = inb(REG_STATUS(0));
        printk("status %02x\n", status);
        if(status & HD_STATUS_ERR) {
            printk("Hard Disk Error.\n");
            break;
        }
        if(!(status & HD_STATUS_BSY) && (status & HD_STATUS_DRQ)) {
            break;
        }
        int i = 1000000000;
        while(i--);
    }

    insw(REG_DATA(0), buf, 256);

    int i;
    for(i=0; i<32; i+=2)
    {
        unsigned char *p;
        unsigned char c;
#if 1
        p = buf+ATA_IDENT_SERIAL;
        c = p[i + 1];
        p[i+1] = p[i];
        p[i] = c;
#endif

        p = buf + ATA_IDENT_MODEL;
        c = p[i + 1];
        p[i+1] = p[i];
        p[i] = c;
    }

    buf[ATA_IDENT_SERIAL+32] = 0;
    buf[ATA_IDENT_MODEL+32] = 0;

    printk("Hard Disk SN: %s Model: %s\n", buf + ATA_IDENT_SERIAL, buf + ATA_IDENT_MODEL);

    free_pages((unsigned long)buf);
}

int iobase;
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
    iobase = v & 0xFFF0;

    pci_write_config_word(2, pci_cmd(pci, PCI_COMMAND));

    v = inw(iobase+0);
    printk(" ide bus master ide command register primary %04x\n", v);
    v = inw(iobase+2);
    printk(" ide bus master ide status register primary %04x\n", v);

    int i;
    printk(" BARS: ");
    for(i=0; i<5; ++i)
        printk("%08x ", pci->bars[i]);
    printk("\n");
#if 0
    prd_t *p = (prd_t *) va2pa(hd_prd_tbl);
    printk("hd_prd_tbl %08x physical %08x sizeof prd %d\n", hd_prd_tbl, p, sizeof(prd_t));
    p->addr = 0;
    p->cnt  = 512;
    p->reserved = 0;
    p->eot = 1;


    outl(p, iobase+4);
    outl(32, iobase+2);
    v = inw(iobase+2);
    printk(" ide bus master ide status register primary %04x\n", v);
#endif

    pci_write_config_byte(0xFE, pci_cmd(pci, PCI_INTRLINE));
    v = pci_read_config_byte(pci_cmd(pci, PCI_INTRLINE));
    printk("---- %x\n", v);
    if((v & 0xFF) == 0xFE) {
        printk("This Device needs IRQ assignment.\n");
        pci_write_config_byte(14, pci_cmd(pci, PCI_INTRLINE));
        v = pci_read_config_byte(pci_cmd(pci, PCI_INTRLINE));
        printk("---- %x\n", v);
    } else {
        if(progif == 0x8A || progif == 0x80) {
            printk("This is a Parallel IDE Controller which use IRQ 14 and IRQ 15.\n");
        }
    }
}

void ide_init()
{
    pci_device_t *pci = pci_find_device(PCI_VENDORID_INTEL, 0x2850);
    if(pci == 0)
        pci = pci_find_device(PCI_VENDORID_INTEL, 0x7010); // qemu

    if(pci == 0)
        panic("can not find ide device");

    printk("found ide pci device\n");

    ide_pci_init(pci);
    outb_p(0x02, REG_CTL(0));
    ide_read_identify();

}

/*
 *--------------------------------------------------------------------------
 *   File Name: hd.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Mon Feb  1 15:26:55 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <io.h>
#include <hd.h>
#include <irq.h>
#include <pci.h>
#include <system.h>


#define IDE_COMMAND 0
#define IDE_STATUS  2
#define IDE_PRDT    4



unsigned long iobase = 0;
//void    hd_handler(pt_regs_t * regs, unsigned int irq)
void hd_handler(unsigned int irq, pt_regs_t * regs, void *dev_id)
{
    //printk("\nhd_handler:%d\n", irq);
    unsigned int v = inw(iobase+2);
    printk("[%04x]", v);
}

int    hd_controller_ready(Dev dev)
{
    int retries = 0x1000;

    do
    {
        if(hd_rdy(dev))
            return 1;
    }while(--retries);

    return 0;
}

void    hd_controller_reset(unsigned int dev)
{
    outb_p(HD_CTL_RESET,    REG_CTL(dev));
    outb_p(0,               REG_CTL(dev));
    while(hd_bsy(dev))
    {
        //printk("bsy");
    }
}

void hd_out(Dev dev, u32 nsect, u64 sect_nr, u32 cmd)
{

    {
    unsigned long long sect_nr = 0;
    unsigned int nsect = 1;

    cli();
    outb_p(0x00, REG_CTL(dev));

    outb(0,           REG_NSECTOR(dev));    // High
    outb((u8)nsect,   REG_NSECTOR(dev));    // Low

    outb((u8)((sect_nr>>24)&0xFF),    REG_LBAL(dev));
    outb((u8)((sect_nr>> 0)&0xFF),    REG_LBAL(dev));

    outb((u8)((sect_nr>>32)&0xFF),    REG_LBAM(dev));
    outb((u8)((sect_nr>> 8)&0xFF),    REG_LBAM(dev));

    outb((u8)((sect_nr>>40)&0xFF),    REG_LBAH(dev));
    outb((u8)((sect_nr>>16)&0xFF),    REG_LBAH(dev));

    outb(0xE0,    REG_DEVSEL(dev));
    outb(0x24,    REG_CMD(dev));
    sti();
    outl(4, iobase);
    return ;
    }
}

void    _hd_read(Dev dev, u64 sect_nr, void *buf, u32 count, u32 cmd)
{
    u32    device;
    u32    nsect;
    u32    retires = 100;

    nsect    = (count + SECT_SIZE -1)/SECT_SIZE;

    do
    {
        hd_out(dev, nsect,  sect_nr, cmd);

        int drq_retires = 100000;
        while(!hd_drq(dev) && --drq_retires)
            /* do nothing */;

        if(drq_retires != 0)
            break;
    }while(--retires);

    if(retires == 0)
        panic("hard disk is not ready");

    hd_rd_data(dev, buf, count);
}


void    hd_read(Dev dev, u64 sect_nr, void *buf, u32 count)
{
    _hd_read(dev, sect_nr, buf, count, HD_CMD_READ_EXT);
}

void    hd_read_identify(Dev dev, void *buf)
{
    _hd_read(dev, 0, buf, SECT_SIZE, HD_CMD_IDENTIFY);
}

typedef struct _ide_ident {
    u16_t   dev_type;
    u8_t    _space_[18];
    u8_t    serial[34];
    u8_t    model[44];
    u8_t    capabilites[8];
    u8_t    filed_valid[4];
} ide_ident_t;

void    hd_print_identify(const char *buf)
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

typedef struct prd
{
    unsigned int addr;
    unsigned int cnt : 16;
    unsigned int reserved : 15;
    unsigned int eot : 1;
} prd_t;

#define PRD_CNT 1
prd_t hd_prd_tbl[PRD_CNT] __attribute__((aligned(64*1024)));

void dump()
{
    unsigned char *p = (unsigned char *) hd_prd_tbl[0].addr;
    int i;
    for(i=510; i<512; ++i)
        printk("[%02x] ", p[i]);
}


void hd_pci_init(pci_device_t *pci)
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


    pci_write_config_byte(0xFE, pci_cmd(pci, PCI_INTRLINE));
    v = pci_read_config_byte(pci_cmd(pci, PCI_INTRLINE));
    printk("---- %x\n", v);
    if((v & 0xFF) == 0xFE) {
        printk("This Device needs IRQ assignment.\n");
    } else {
        if(progif == 0x8A || progif == 0x80) {
            printk("This is a Parallel IDE Controller which use IRQ 14 and IRQ 15.\n");
        }
    }

    pci_write_config_byte(14, pci_cmd(pci, PCI_INTRLINE));
    v = pci_read_config_byte(pci_cmd(pci, PCI_INTRLINE));
    printk("---- %x\n", v);
}

/*
 * Bus Master IDE Status Register
 * Bit2 Bit0
 *   0   0      Error Condition
 *   0   1      DMA transfer is in progress
 *   1   0      The IDE device generated an interrupt and the Physical Region Descriptors exhausted
 *   1   1      The IDE device generated an interrupt
 */

void setup_hd()
{
#if 1
    pci_device_t *pci = pci_find_device(PCI_VENDORID_INTEL, 0x2850);
    if(pci == 0)
        pci = pci_find_device(PCI_VENDORID_INTEL, 0x7010); // qemu

#else
    pci_device_t *pci = pci_find_device(PCI_VENDORID_INTEL, 0x2922);
#endif

    if(pci == 0)
        panic("can not find ide device");

    printk("found ide pci device\n");

    hd_pci_init(pci);

    hd_controller_reset(ROOT_DEV);

    return ;

    char *buf;
    buf = (unsigned char *) alloc_one_page(0);
    assert(buf != NULL);

    hd_read_identify(ROOT_DEV, buf);

    hd_print_identify(buf);

    free_pages((unsigned long)buf);
}

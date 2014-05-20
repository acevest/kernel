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
//void    hd_handler(pt_regs_t * regs, unsigned int irq)
void hd_handler(unsigned int irq, pt_regs_t * regs, void *dev_id)
{
    printk("\nhd_handler:%d\n", irq);
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
    outb_p(0,    REG_CTL(dev));
    while(hd_bsy(dev))
    {
        printk("bsy");
    }
}

void hd_out(Dev dev, u32 nsect, u64 sect_nr, u32 cmd)
{
    assert(nsect > 0);
    assert(HD_GET_DEV(dev) < 2);    // 0: ata-master 1:ata-slave

    unsigned char device = 0xE0;
    device |= ((HD_GET_DEV(dev)) << 4);

    if(!hd_controller_ready(dev))
    {
        printk("hd is not ready\n");
        return ;
    }

    if(system.debug)
    {
        hd_controller_reset(dev);
    }

    printk("hdout\n");
    outb(0x00,                REG_CTL(dev));
    outb(0x00,                REG_FEATURES(dev));
    outb((u8)nsect,           REG_NSECTOR(dev));
#ifdef USE_LBA_48
    /* 
     * LBA-48 bit
     * 先写高位.再写低位.
     */
    outb((u8)((sect_nr>>0x18)&0xFF),    REG_LBAL(dev));
    outb((u8)((sect_nr>>0x20)&0xFF),    REG_LBAM(dev));
    outb((u8)((sect_nr>>0x28)&0xFF),    REG_LBAH(dev));

    outb((u8)((sect_nr>>0x00)&0xFF),    REG_LBAL(dev));
    outb((u8)((sect_nr>>0x08)&0xFF),    REG_LBAM(dev));
    outb((u8)((sect_nr>>0x10)&0xFF),    REG_LBAH(dev));
#else
    outb((u8)((sect_nr>>0x00)&0xFF),    REG_LBAL(dev));
    outb((u8)((sect_nr>>0x08)&0xFF),    REG_LBAM(dev));
    outb((u8)((sect_nr>>0x10)&0xFF),    REG_LBAH(dev));
#endif    
    outb((u8)device,    REG_DEVICE(dev));
    outb((u8)cmd,       REG_CMD(dev));

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

void hd_pci_init(pci_device_t *pci)
{
    unsigned int v;
    v = pci_read_config_word(pci_cmd(pci, PCI_COMMAND));
    printk(" ide pci command %04x\n", v);
    v = pci_read_config_byte(pci_cmd(pci, PCI_PROGIF));
    printk(" ide pci program interface %02x\n", v);
    v = pci_read_config_long(pci_cmd(pci, PCI_BAR4));
    printk(" ide pci Base IO Address Register %08x\n", v);
    unsigned long iobase = v & 0xFFF0;

    v = inw(iobase+0);
    printk(" ide bus master ide command register primary %04x\n", v);
    v = inw(iobase+2);
    printk(" ide bus master ide status register primary %04x\n", v);
}

void setup_hd()
{
    pci_device_t *pci = pci_find_device(PCI_VENDORID_INTEL, 0x2850);
    if(pci == 0)
        pci = pci_find_device(PCI_VENDORID_INTEL, 0x7010); // qemu

    if(pci == 0)
        panic("can not find ide device");

    printk("found ide pci device\n");

    hd_pci_init(pci);

    hd_controller_reset(ROOT_DEV);

    char *buf;
    buf = (unsigned char *) alloc_one_page(0);
    assert(buf != NULL);

    hd_read_identify(ROOT_DEV, buf);

    hd_print_identify(buf);

    free_pages((unsigned long)buf);
}

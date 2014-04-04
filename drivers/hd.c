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
#include <system.h>
//void    hd_handler(pPtRegs regs, unsigned int irq)
void    hd_handler(unsigned int irq, pPtRegs regs, void *dev_id)
{
    printk("hd_handler:%d ", irq);
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
    outb(HD_CTL_RESET,    REG_CTL(dev));
/*
    //outb(HD_CTL_RESET,    REG_CTL(dev));
    while(1)
    {
        printk("%02x ", inb(REG_STATUS(dev)));
    }
    while(hd_bsy(dev))
    {
        printk("bsy");
    }
*/
}

void    hd_out(Dev dev, u32 nsect, u64 sect_nr, u32 cmd)
{
    assert(nsect > 0);
    assert(HD_GET_DEV(dev) < 2);    // 0: ata-master 1:ata-slave

    unsigned char device = 0xE0;
    device |= ((HD_GET_DEV(dev)) << 4);

    if(!hd_controller_ready(dev))
    {
        return ;
    }

    if(system.debug)
    {
        hd_controller_reset(dev);
    }

    outb(0x00,                REG_CTL(dev));
    outb(0x00,                REG_FEATURES(dev));
    outb((u8)nsect,                REG_NSECTOR(dev));
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
    outb((u8)device,            REG_DEVICE(dev));
    outb((u8)cmd,                REG_CMD(dev));
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
    short    hd_capabilites = ident[49];
    short     hd_supt_inst_set = ident[83];

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

void    setup_hd()
{
    if(!system.debug)
        hd_controller_reset(ROOT_DEV);
    //hd_controller_reset(1);
    //return ;

    char *buf;
    buf = (unsigned char *) get_virt_pages(1);
    assert(buf != NULL);

#if 1
    hd_read_identify(ROOT_DEV, buf);
#else
    outb(0x00, 0x1F1);
    outb(0x01, 0x1F2);
    outb(0x00, 0x1F3);
    outb(0x00, 0x1F4);
    outb(0x00, 0x1F5);
    outb(0xE0, 0x1F6);
    outb(0xEC, 0x1F7);
    while(!(inb(0x1F7) & 0x08)){}
    asm("cld;rep;insw;"::"c"(256), "d"(0x1F0), "D"(buf));
#endif
    hd_print_identify(buf);

    free_virt_pages(buf);
}




#if 0
int    hd_controller_ready(unsigned int channel)
{
    int retries = 0x1000;

    do
    {
        if(hd_rdy(channel))
            return 1;
    }while(--retries);

    return 0;
}

void    hd_controller_reset(unsigned int channel)
{
    outb(HD_CTL_RESET,    REG_CTL(channel));
}

void    hd_out(u32 channel, u32 device, u32 nsect, u64 sect_nr, u32 cmd)
{
    assert(device < 2);    // 0: ata-master 1: ata-slave
    assert(channel < 2);    // PC only support 2 IDE channels
    assert(nsect > 0);

    unsigned char dev = (device)?0x10 : 0x00;
    dev |= 0xE0;

    if(!hd_controller_ready(channel))
        return ;

    outb(0x00,                REG_CTL(channel));
    outb(0x00,                REG_FEATURES(channel));
    outb((u8)nsect,                REG_NSECTOR(channel));
#ifdef USE_LBA_48
    /* 
     * LBA-48 bit
     * 先写高位.再写低位.
     */
    outb((u8)((sect_nr>>0x18)&0xFF),    REG_LBAL(channel));
    outb((u8)((sect_nr>>0x20)&0xFF),    REG_LBAM(channel));
    outb((u8)((sect_nr>>0x28)&0xFF),    REG_LBAH(channel));

    outb((u8)((sect_nr>>0x00)&0xFF),    REG_LBAL(channel));
    outb((u8)((sect_nr>>0x08)&0xFF),    REG_LBAM(channel));
    outb((u8)((sect_nr>>0x10)&0xFF),    REG_LBAH(channel));
#else
    outb((u8)((sect_nr>>0x00)&0xFF),    REG_LBAL(channel));
    outb((u8)((sect_nr>>0x08)&0xFF),    REG_LBAM(channel));
    outb((u8)((sect_nr>>0x10)&0xFF),    REG_LBAH(channel));
#endif    
    outb((u8)dev,                REG_DEVICE(channel));
    outb((u8)cmd,                REG_CMD(channel));
}

void    _hd_read(u32 dev, u64 sect_nr, void *buf, u32 count, u32 cmd)
{
    u32    channel, device;
    u32    nsect;
    u32    retires = 100;

    channel = device = 0;    //

    nsect    = (count + SECT_SIZE -1)/SECT_SIZE;

    do
    {
        hd_out(channel, device,    nsect,  sect_nr, cmd);

        int drq_retires = 100000;
        while(!hd_drq(channel) && --drq_retires)
            /* do nothing */;

        if(drq_retires != 0)
            break;
    }while(--retires);

    if(retires == 0)
        panic("hard disk is not ready");

    hd_rd_data(channel, buf, count);
}


void    hd_read(u32 dev, u64 sect_nr, void *buf, u32 count)
{
    _hd_read(dev, sect_nr, buf, count, HD_CMD_READ_EXT);
}

void    hd_read_identify(u32 dev, void *buf)
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
    short    hd_capabilites = ident[49];
    short     hd_supt_inst_set = ident[83];

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


    printk("Hard Disk SN: %s\n", hd_sn);
    printk("Hard Disk Model: %s\n", hd_model);
    printk("Hard Disk Support LBA: %s\n",
        (hd_capabilites & 0x0200) ? "Yes" : "No");
    printk("Hard Disk Support LBA-48bit: %s\n",
        (hd_supt_inst_set & 0x0400) ? "Yes" : "No");

    if(!(hd_supt_inst_set & 0x0400))
        panic("Your hard disk ");
}

void    setup_hd()
{
    //hd_controller_reset(0);
    //hd_controller_reset(1);
    //return ;

    char *buf;
    buf = (unsigned char *) get_virt_pages(1);
    assert(buf != NULL);

#if 1
    hd_read_identify(ROOT_DEV, buf);
#else
    outb(0x00, 0x1F1);
    outb(0x01, 0x1F2);
    outb(0x00, 0x1F3);
    outb(0x00, 0x1F4);
    outb(0x00, 0x1F5);
    outb(0xE0, 0x1F6);
    outb(0xEC, 0x1F7);
    while(!(inb(0x1F7) & 0x08)){}
    asm("cld;rep;insw;"::"c"(256), "d"(0x1F0), "D"(buf));
#endif
    hd_print_identify(buf);

    free_virt_pages(buf);
}
#endif


#if 0
void    hd_rd_sect(u64 sn, char *buf)
{
    int chl = 0;
    unsigned char dev = 0xE0;

    outb(HD_CTL_RESET,    REG_CTL(chl));
    outb(HD_CTL_DISABLE_INT,REG_CTL(chl));
    outb(0x00,         REG_FEATURES(chl));
    outb(0x01,        REG_NSECTOR(chl));
#if 0
    outb((sn>>0x00)&0xFF,    REG_LBAL(chl));
    outb((sn>>0x08)&0xFF,    REG_LBAM(chl));
    outb((sn>>0x10)&0xFF,    REG_LBAH(chl));
#endif
#if 0
    outb(0x00,    REG_LBAL(chl));
    outb(0x00,    REG_LBAL(chl));
    outb(0x00,    REG_LBAM(chl));
    outb(0x00,    REG_LBAM(chl));
    outb(0x00,    REG_LBAH(chl));
    outb(0x00,    REG_LBAH(chl));
#endif
#if 1
    /* 
     * LBA-48 bit
     * 先写高位.再写低位.
     */
    outb(0x00,            REG_LBAL(chl));
    outb(0x00,            REG_LBAM(chl));
    outb(0x00,            REG_LBAH(chl));

    outb(0x00,            REG_LBAL(chl)); outb(0x00,            REG_LBAM(chl));
    outb(0x00,            REG_LBAH(chl));
#endif

    outb(dev,            REG_DEVICE(chl));
    outb(HD_CMD_READ_EXT,        REG_CMD(chl));

    while(!hd_drq(chl));

    hd_rd_data(chl, buf, SECT_SIZE);
}
#endif

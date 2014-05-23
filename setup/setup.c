/*
 *--------------------------------------------------------------------------
 *   File Name: setup.c
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Wed Mar  4 20:34:36 2009
 * Last Update: Wed Mar  4 20:34:36 2009
 * 
 *--------------------------------------------------------------------------
 */
#include <bits.h>
#include <printk.h>
#include <system.h>
#include <io.h>
#include <hd.h>

extern void setup_gdt();
extern void setup_idt();
extern void setup_gate();
extern void detect_cpu();
extern void setup_sysc();
extern void setup_pci();
extern void set_tss();
extern void show_logo();
extern void setup_tasks();
extern void setup_root_dev();
extern void setup_hd();
extern void setup_fs();
extern void setup_ext2();

extern void reboot();

#define HZ 100
#define CLOCK_TICK_RATE 1193180
#define LATCH ((CLOCK_TICK_RATE + HZ/2) / HZ)

void setup_i8253()
{
    outb_p(0x34, 0x43);
    outb_p(LATCH & 0xFF, 0x40);
    outb(LATCH >> 8, 0x40);
}

#define VERSION    "0.2.5 Final"
#define BUIDER    "Zhao Yanbai"
const char *version = 
    "Kernel "
    VERSION
    " Build on "__DATE__ " " __TIME__
    " by "
    BUIDER;


extern void hd_out(Dev dev, u32 nsect, u64 sect_nr, u32 cmd);

#include "hd.h"
void hd()
{
    unsigned long long sect_nr = 0;
    unsigned int nsect = 1;

    cli();
    outb(0x00, REG_CTL(dev));

    outb(0,           REG_NSECTOR(dev));    // High
    outb((u8)nsect,   REG_NSECTOR(dev));    // Low

    outb((u8)((sect_nr>>24)&0xFF),    REG_LBAL(dev));
    outb((u8)((sect_nr>> 0)&0xFF),    REG_LBAL(dev));

    outb((u8)((sect_nr>>32)&0xFF),    REG_LBAM(dev));
    outb((u8)((sect_nr>> 8)&0xFF),    REG_LBAM(dev));

    outb((u8)((sect_nr>>40)&0xFF),    REG_LBAH(dev));
    outb((u8)((sect_nr>>16)&0xFF),    REG_LBAH(dev));

    outb(0xE0,    REG_DEVICE(dev));
    outb(0x24,    REG_CMD(dev));
    sti();

    int drq_retires = 100000;
    while(!hd_drq(dev) && --drq_retires)
        /* do nothing */;

    if(drq_retires == 0)
        printk("hard disk no ready\n");
    else
        printk("read finished\n");

    char buf[1024];
    memset(buf, 0, 1024);
    hd_rd_data(0, buf, 512);

    printk("SECTOR %04x\n", *(unsigned short *)(buf+510));

    while(1);
}


void setup_kernel()
{
    extern char kernel_begin, kernel_end;

    printk("kernel: %08x - %08x\n", &kernel_begin, &kernel_end);

    init_mm();

    setup_gdt();
    setup_idt();
    setup_gate();

    setup_i8253();

    detect_cpu();

    set_tss();

    setup_sysc();
    //setup_pci();

    setup_tasks();

    setup_irqs();

    asm("sti;");
    hd();

    setup_hd();
    printk("%s\n", version);
    //asm("cli;");
    //while(1);
    return;
    hd_out(0, 1, 1, HD_CMD_READ_EXT);
    while(1); // TODO MODIFY CODE BELOW


    setup_root_dev();
    setup_fs();
    setup_ext2();

    show_logo();
}


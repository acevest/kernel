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
extern void ide_init();
extern void setup_fs();
extern void setup_ext2();

extern void reboot();
extern void cnsl_init();
extern void vga_init();

#define HZ 10
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


void setup_kernel()
{
    extern char kernel_begin, kernel_end;

    vga_init();

    printk("kernel: %08x - %08x\n", &kernel_begin, &kernel_end);

    init_mm();
    //while(1);
    setup_gdt();
    setup_idt();
    setup_gate();

    setup_i8253();

    detect_cpu();

    set_tss();

    setup_sysc();
    setup_pci();

    cnsl_init();


    setup_tasks();

    setup_irqs();
    
    void ide_init();
    ide_init();
    printk("%s\n", version);


    return;
    while(1); // TODO MODIFY CODE BELOW


    setup_root_dev();
    setup_fs();
    setup_ext2();

    show_logo();
}


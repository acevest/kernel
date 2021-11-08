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
#include <io.h>
#include <printk.h>
#include <system.h>
#include <tty.h>

extern void init_mm();
extern void setup_gdt();
extern void setup_idt();
extern void setup_gate();
extern void detect_cpu();
extern void setup_sysc();
extern void setup_pci();
extern void set_tss();
extern void setup_tasks();
extern void setup_irqs();
extern void ide_init();
extern void setup_fs();
extern void setup_ext2();

extern void reboot();
extern void cnsl_init();
extern void init_ttys();

#define HZ 10
#define CLOCK_TICK_RATE 1193180
#define LATCH ((CLOCK_TICK_RATE + HZ / 2) / HZ)

void setup_i8253() {
    outb_p(0x34, 0x43);
    outb_p(LATCH & 0xFF, 0x40);
    outb(LATCH >> 8, 0x40);
}

#define VERSION "0.3.1"
const char *version = "Kernel version " VERSION " @ " BUILDER
                      " ["__DATE__
                      " " __TIME__
                      "]"

                      "\n";

void setup_kernel() {
    init_ttys();

    init_mm();

    // printk("kernel: %08x - %08x\n", system.kernel_begin, system.kernel_end);

    setup_gdt();
    setup_idt();
    setup_gate();

    setup_i8253();

    set_tss();

    setup_sysc();

    cnsl_init();

    printl(MPL_TITLE, "                                 SYSTEM MONITOR");
    printl(MPL_ROOTDEV, "root device %08x", system.root_dev);

    system_delay();

    setup_tasks();

    setup_irqs();
    extern tty_t monitor_tty;
    // tty_switch(&monitor_tty);
    return;

    // switch_printk_screen();
    setup_pci();
    // switch_printk_screen();
    system_delay();
    void ide_init();
    ide_init();

    system_delay();

    detect_cpu();

    setup_fs();

    printk(version);

    // switch_printk_screen();
}

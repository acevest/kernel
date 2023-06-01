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
#include <string.h>
#include <system.h>
#include <tty.h>

extern void init_mm();
extern void setup_gdt();
extern void setup_idt();
extern void setup_gate();
void setup_i8253(uint16_t hz);
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

#define VERSION "0.3.1"
const char *version = "KERNEL v" VERSION " @" BUILDER
                      " ["__DATE__
                      " " __TIME__
                      "]"
                      "\n\n";

void setup_kernel() {
    printk("sysenter esp mode: %s\n",
#if FIX_SYSENTER_ESP_MODE
           "fixed to &tss.esp0"
#else
           "use task union stack"
#endif
    );

    init_mm();

    // printk("kernel: %08x - %08x\n", system.kernel_begin, system.kernel_end);
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    setup_sysc();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    cnsl_init();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    const char *title = "KERNEL MONITOR";
    printlxy(MPL_TITLE, (80 - strlen(title)) / 2, title);

    setup_tasks();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    setup_pci();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    detect_cpu();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    printk(version);
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    extern tty_t monitor_tty;
    // tty_switch(&monitor_tty);

    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    setup_i8253(100);
    setup_irqs();

    void ide_init();
    ide_init();
}

// 在开中断的情况下继续初始化的内容
void setup_under_irq() {
    void ata_init();
    ata_init();
    return;
    setup_fs();
}

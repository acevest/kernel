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
extern void init_buffer();
extern void setup_gdt();
extern void setup_idt();
extern void setup_gate();
void setup_i8254(uint16_t hz);
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
const char* version = "KERNEL v" VERSION " @" BUILDER " [" __DATE__ " " __TIME__
                      "]"
                      "\n\n";

void print_kernel_version() {
    //
    extern tty_t* const default_tty;
    tty_t* const tty = default_tty;

    int len = strlen(version);

    for (int i = 0; i < tty->max_x; i++) {
        char c = i < len ? version[i] : ' ';
        c = c != '\n' ? c : ' ';
        c = c != '\t' ? c : ' ';

        //
        uint32_t fg_color = tty->fg_color;
        uint32_t bg_color = tty->bg_color;

        fg_color = TTY_WHITE | TTY_FG_HIGHLIGHT;
        bg_color = TTY_CYAN;

        //
        char* dst = (char*)tty->base_addr;

        //
        dst[i * 2 + 0] = c;
        dst[i * 2 + 1] = ((bg_color) << 4) | (fg_color);
    }

    //
    printk(version);
}

void parse_rsdt(void* addr);

void setup_kernel() {
    printk("sysenter esp mode: fixed to &tss.esp0\n");

    init_mm();

    init_buffer();
#if 0
    parse_rsdt(system.rsdt_addr);
#endif
#if 1
    void init_apic();
    init_apic();
#endif

    void init_mount();
    init_mount();

    // printk("kernel: %08x - %08x\n", system.kernel_begin, system.kernel_end);
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    setup_sysc();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    cnsl_init();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    const char* title = "KERNEL MONITOR";
    printlxy(MPL_TITLE, (80 - strlen(title)) / 2, title);

    setup_fs();

    setup_tasks();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    void mount_root();
    mount_root();

    setup_pci();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    detect_cpu();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    print_kernel_version();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    extern tty_t* const monitor_tty;
    // tty_switch(monitor_tty);

    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    setup_i8254(100);
    setup_irqs();

    void ide_init();
    ide_init();

    void init_sata();
    init_sata();

    void dump_fixmap();
    dump_fixmap();
}

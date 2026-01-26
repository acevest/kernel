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
#include <hpet.h>

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

void prepare_ap_code(paddr_t paddr) {
    // 注意: 最开始时AP是运行在实模式
    paddr += KERNEL_VADDR_BASE;
    // *(volatile uint8_t*)(paddr + 0) = 0x90;
    // *(volatile uint8_t*)(paddr + 1) = 0x90;
    // *(volatile uint8_t*)(paddr + 2) = 0x90;
    // *(volatile uint8_t*)(paddr + 3) = 0xEA;     // jmp
    // *(volatile uint16_t*)(paddr + 4) = 0x0000;  // offset: 0000
    // *(volatile uint16_t*)(paddr + 6) = 0x0100;  // cs:0100

    extern char ap_boot_bgn;
    extern char ap_boot_end;
    uint32_t bytes = &ap_boot_end - &ap_boot_bgn;

    for (int i = 0; i < bytes; i++) {
        ((uint8_t*)paddr)[i] = ((uint8_t*)&ap_boot_bgn)[i];
    }

    // 修正代码里跳入保护模式的地址和gdtr里的gdt的base地址
    extern uint8_t ap_code32_entry_address;
    extern uint8_t ap_gdtr_base;

    uint32_t* dst = 0;

    //
    dst = (uint32_t*)(paddr + (uint32_t)(&ap_code32_entry_address) - (uint32_t)(&ap_boot_bgn));
    (*dst) -= (uint32_t)(&ap_boot_bgn);
    (*dst) += (paddr - KERNEL_VADDR_BASE);

    //
    dst = (uint32_t*)(paddr + (uint32_t)(&ap_gdtr_base) - (uint32_t)(&ap_boot_bgn));
    (*dst) -= (uint32_t)(&ap_boot_bgn);
    (*dst) += (paddr - KERNEL_VADDR_BASE);
}

void wait_ap_boot() {
    paddr_t ap_code_addr = 0x1000;
    prepare_ap_code(ap_code_addr);

    void wakeup_ap(paddr_t paddr);
    wakeup_ap(ap_code_addr);

    printk("wait AP ready...\n");
    extern bool ap_ready();
    while (!ap_ready()) {
        asm("pause");
    }
    printk("AP ready\n");
}

void setup_kernel() {
    printk("sysenter esp mode: fixed to &tss.esp0\n");

    init_mm();

    set_printk(_printk);

    init_buffer();

    void init_mount();
    init_mount();

    // printk("kernel: %08x - %08x\n", system.kernel_begin, system.kernel_end);
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    setup_sysc();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    cnsl_init();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

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

    // extern tty_t* const monitor_tty;
    // tty_switch(monitor_tty);

    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    setup_i8254(100);
    setup_irqs();

#if 1
    void init_acpi();
    init_acpi();
#endif
#if 1
    void init_apic();
    init_apic();
#endif

#if 1
    hpet_init();
#endif

    // ap 启动需要用到 hpet来校准
    wait_ap_boot();

    hpet_init_timer0(101);

#if !DISABLE_IDE
    void ide_init();
    ide_init();
#endif
    void init_ahci();
    init_ahci();

    void dump_fixmap();
    dump_fixmap();
}

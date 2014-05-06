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

extern unsigned long mb_mm_lower, mb_mm_upper;
extern unsigned long mb_mmap_addr, mb_mmap_size;

extern void reboot();


void setup_kernel()
{
    extern char kernel_begin, kernel_end;

    printk("kernel: %08x - %08x\n", &kernel_begin, &kernel_end);

    init_mm();

    setup_gdt();
    setup_idt();
    setup_gate();

    detect_cpu();

    set_tss();

    setup_sysc();
    //setup_pci();

    setup_irqs();

    setup_tasks();

    return;
    while(1); // TODO MODIFY CODE BELOW


    setup_root_dev();
    setup_hd();
    setup_fs();
    setup_ext2();

    show_logo();
}


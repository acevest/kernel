/*
 *--------------------------------------------------------------------------
 *   File Name: keyboard.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Thu Jul 16 18:39:57 2009
 * Last Update: Thu Jul 16 18:39:57 2009
 *
 *--------------------------------------------------------------------------
 */
#include <console.h>
#include <io.h>
#include <printk.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include <system.h>
#include <tty.h>

void reboot();
void poweroff();
void ide_debug();
void ide_status();
void debug_sched();
void vga_dbg_toggle();
int debug_wait_queue_put(unsigned int v);
void ide_dma_pci_lba48();
void vga_switch(unsigned int nr);

void kbd_debug(unsigned char scan_code);

char kbd_char_tbl[] = {
    0,   0,   '1', '2', '3', '4', '5',  '6', '7', '8', '9', '0', '-', '=', '\b', 0,   'q', 'w', 'e',  'r', 't', 'y',
    'u', 'i', 'o', 'p', '[', ']', '\n', 0,   'a', 's', 'd', 'f', 'g', 'h', 'j',  'k', 'l', ';', '\'', '`', 0,   '\\',
    'z', 'x', 'c', 'v', 'b', 'n', 'm',  ',', '.', '/', 0,   0,   0,   ' ', 0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,    0,   0,   0,
    0,   0,   0,   0,   0,   0,   0,    0,   0,   0,   0,   0,   0,   0,
};

void kbd_handler(unsigned int irq, pt_regs_t *regs, void *dev_id) {
    unsigned char scan_code;

    scan_code = inb(0x60);

    kbd_debug(scan_code);

    if (0x80 & scan_code)  // break code
    {
        return;
    }

    unsigned int inx = scan_code & 0xFF;
    char ch = kbd_char_tbl[inx];
    cnsl_kbd_write(ch);
}

void kbd_debug(unsigned char scan_code) {
    static unsigned long kbd_cnt = 0;
    printl(MPL_KEYBOARD, "keyboard irq: %d scan code %02x", kbd_cnt++, scan_code);

    if (scan_code == 0x01) {  // Esc
        reboot();
    }

    printd("[%02x]", scan_code);

    // if (scan_code == 0x3B)  // F1
    //     vga_switch(0);
    // if (scan_code == 0x3C)  // F2
    //     vga_switch(1);
    // if (scan_code == 0x3D)  // F3
    //     vga_switch(2);
    // if (scan_code == 0x3E)  // F4
    //     vga_switch(3);

    if (scan_code == 0x3F)  // F5
        debug_wait_queue_put(0);
    if (scan_code == 0x40)  // F6
        debug_wait_queue_put(1);
    if (scan_code == 0x41)  // F7
        debug_wait_queue_put(2);
    if (scan_code == 0x42)  // F8
        debug_wait_queue_put(7);

    if (scan_code == 0x43)  // F9
        ide_dma_pci_lba48();
    if (scan_code == 0x44)  // F10
        ide_debug();
    if (scan_code == 0x57)  // F11
    {
        asm("cli;");
        while (1)
            ;
    }

    extern tty_t default_tty;
    extern tty_t monitor_tty;
    if (scan_code == 0x58) {  // F12
        current_tty = current_tty != &default_tty ? &default_tty : &monitor_tty;
        tty_switch(current_tty);
    }

    // ide_status();
}

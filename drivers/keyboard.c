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

void kbd_debug(uint8_t scan_code);

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

// TODO 改造成环形缓冲区
uint8_t kbd_scan_code;
void kbd_bh_handler(void* arg) {
    kbd_debug(kbd_scan_code);
    if (0x80 & kbd_scan_code) {  // break code
        return;
    }
    uint8_t inx = kbd_scan_code & 0xFF;
    char ch = kbd_char_tbl[inx];
    cnsl_kbd_write(ch);
}

void kbd_handler(unsigned int irq, pt_regs_t* regs, void* dev_id) {
    kbd_scan_code = inb(0x60);
    add_irq_bh_handler(kbd_bh_handler, NULL);
}

extern tty_t* const default_tty;
extern tty_t* const monitor_tty;
extern tty_t* const debug_tty;
extern void tty_switch_to_next();

uint64_t kbd_irq_cnt = 0;
void kbd_debug(uint8_t scan_code) {
    kbd_irq_cnt++;

    if (scan_code == 0x01) {  // Esc
        // reboot();
    }

    // printd("[%02x]", scan_code);

    if (scan_code == 0x3B) {  // F1
        tty_switch(default_tty);
    } else if (scan_code == 0x3C) {  // F2
        tty_switch(monitor_tty);
    } else if (scan_code == 0x3D) {  // F3
        tty_switch(debug_tty);
    }

    if (scan_code == 0x43) {  // F9
        void ata_test(uint64_t nr);
        ata_test(0);
    }
    if (scan_code == 0x44) {  // F10
        void ata_send_read_identify_cmd(int dev);
        ata_send_read_identify_cmd(0);
    }

    if (scan_code == 0x57)  // F11
    {
        asm("cli;");
        while (1)
            ;
    }

    if (scan_code == 0x58) {  // F12
        tty_switch_to_next();
    }

    // ide_status();
}

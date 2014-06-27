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
#include <printk.h>
#include <system.h>
#include <syscall.h>
#include <stdio.h>
#include <io.h>
#include <console.h>

void reboot();
void poweroff();
void ide_debug();
void ide_status();
void debug_sched();
void vga_dbg_toggle();
int debug_wait_queue_put(unsigned int v);

unsigned long kbd_cnt = 0;
void kbd_handler(unsigned int irq, pt_regs_t * regs, void *dev_id)
{
    unsigned char scan_code;
    scan_code = inb(0x60);

    printd(MPL_KEYBOARD, "keyboard:%d scan code %02x", kbd_cnt++, scan_code);

    if(scan_code == 0x01) // Esc
        reboot();
    
    printk("[%02x]", scan_code);

    if(scan_code == 0x13)   // r
        ide_debug();

    if(scan_code == 0x1F)   // s
        ide_status();

    if(scan_code == 0x14)   // t
        debug_sched();

    if(scan_code == 0x3B)   // F1
        vga_switch(0);
    if(scan_code == 0x3C)   // F2
        vga_switch(1);
    if(scan_code == 0x3D)   // F3
        vga_switch(2);
    if(scan_code == 0x3E)   // F4
        vga_switch(3);

    if(scan_code == 0x3F)   // F5
        debug_wait_queue_put(0);
    if(scan_code == 0x40)   // F6
        debug_wait_queue_put(1);
    if(scan_code == 0x41)   // F7
        debug_wait_queue_put(2);
    if(scan_code == 0x42)   // F8
        debug_wait_queue_put(7);

    if(scan_code == 0x43);  // F9
    if(scan_code == 0x44);  // F10
    if(scan_code == 0x57);  // F11
    if(scan_code == 0x58)   // F12
        vga_dbg_toggle();

#if 1
    cnsl_rd_q.data[0] = (char) scan_code;
    wake_up(&cnsl_rd_q.wait);
#endif
}

int sysc_read_kbd()
{
    DECLARE_WAIT_QUEUE(wait, current);
    add_wait_queue(&cnsl_rd_q.wait, &wait);


    return 0;
}

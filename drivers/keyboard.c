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
void vga_toggle();

void kbd_handler(unsigned int irq, pt_regs_t * regs, void *dev_id)
{
    unsigned char scan_code;
    scan_code = inb(0x60);

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
        vga_toggle();

    if((cnsl_rd_q.head+1) == cnsl_rd_q.tail)
        goto end;

    cnsl_rd_q.data[cnsl_rd_q.head++] = (char) scan_code;

end:

    wake_up(&cnsl_rd_q.wait);
}

int sysc_read_kbd()
{
    DECLARE_WAIT_QUEUE(wait, current);
    add_wait_queue(&cnsl_rd_q.wait, &wait);


    return 0;
}

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

#define KBD_BUF_SIZE    256
static struct
{
    unsigned int count;
    unsigned int head,tail;
    unsigned char buf[KBD_BUF_SIZE];
} kbd_buf;
#define count    kbd_buf.count
#define head    kbd_buf.head
#define tail    kbd_buf.tail
#define buf    kbd_buf.buf



extern void reboot();
extern void poweroff();
extern void ide_debug();
extern void ide_status();
void    kbd_handler(unsigned int irq, pt_regs_t * regs, void *dev_id)
{
    unsigned char scan_code;
    scan_code = inb(0x60);

    if(scan_code == 0x01) // Esc
        reboot();
    
    printk("%02x", scan_code);

    if(scan_code == 0x13)   // r
        ide_debug();

    if(scan_code == 0x1F)
        ide_status();

    if(count < KBD_BUF_SIZE)
    {
        count++;
        buf[tail++] = scan_code;
        tail %= KBD_BUF_SIZE;
    }
}

inline int getscan_code()
{
    unsigned int scan_code;
    
    //while(count <= 0);
    if(count <= 0) return -1;

    scan_code = buf[head++];
    head %= KBD_BUF_SIZE;
    count--;    //很明显这是临界资源但现在只能这样了

    return (0xFF & scan_code);
}


int    sysc_read_kbd()
{
    return getscan_code();
}

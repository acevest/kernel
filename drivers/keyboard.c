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
#include <hd.h>

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
extern void hd_out(Dev dev, u32 nsect, u64 sect_nr, u32 cmd);
void    kbd_handler(unsigned int irq, pt_regs_t * regs, void *dev_id)
{
    unsigned char ScanCode;
    //printk("%s\n", dev_id);
    ScanCode = inb(0x60);

    if(ScanCode == 0x01) // Esc
        reboot();
    
    printk("%02x", ScanCode);

    hd_out(0, 1, 1, HD_CMD_READ_EXT);

    if(count < KBD_BUF_SIZE)
    {
        count++;
        buf[tail++] = ScanCode;
        tail %= KBD_BUF_SIZE;
    }
}

inline int getScanCode()
{
    unsigned int ScanCode;
    
    //while(count <= 0);
    if(count <= 0) return -1;

    ScanCode = buf[head++];
    head %= KBD_BUF_SIZE;
    count--;    //很明显这是临界资源但现在只能这样了

    return (0xFF & ScanCode);
}


int    sysc_read_kbd()
{
    return getScanCode();
}

/*
 *--------------------------------------------------------------------------
 *   File Name: printk.c
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0 Wed Jul 30 14:25:23 2008
 *     Version: 1.1 Tue Feb 10 22:40:25 2009
 *     Version:    2.0 Sun Mar 08 10:52:51 2009
 *     Version:    3.0 Sat Jul 18 23:06:27 2009
 *
 *--------------------------------------------------------------------------
 */

#include <system.h>
#include <tty.h>

extern void vga_puts(unsigned int nr, const char *buf, unsigned char color);
extern void vga_dbg_puts(unsigned int line, unsigned int offset, const char *buf);
extern int vsprintf(char *buf, const char *fmt, char *args);

unsigned int printk_screen_nr = 0;

extern unsigned int vga_screen_cnt();
void switch_printk_screen() {
    printk_screen_nr++;
    printk_screen_nr %= vga_screen_cnt();
}

char pkbuf[1024];
extern tty_t default_tty;
int printk(const char *fmtstr, ...) {
    char *args = (char *)(((char *)&fmtstr) + 4);
    int size = vsprintf(pkbuf, fmtstr, args);
    // vga_puts(printk_screen_nr, pkbuf, 0x2);
    tty_write(&default_tty, pkbuf, (size_t)size);
    // tty_write(&default_tty, "aaa\n", (size_t)4);
    return 0;
}

int printd(const char *fmtstr, ...) {
    char *pdbuf = (char *)kmalloc(1024, 0);
    char *args = (char *)(((char *)&fmtstr) + 4);
    vsprintf(pdbuf, fmtstr, args);
    vga_puts(3, pdbuf, 0x4);
    kfree(pdbuf);
    return 0;
}

char plobuf[1024];
extern tty_t monitor_tty;
int printlo(unsigned int line, unsigned int offset, const char *fmtstr, ...) {
    char *args = (char *)(((char *)&fmtstr) + 4);
    int size = vsprintf(plobuf, fmtstr, args);
    // vga_dbg_puts(line, offset, plobuf);
    tty_write_at(&monitor_tty, offset, line, plobuf, (size_t)size);
    // tty_write_at(&monitor_tty, 1, 2, "abc\n", (size_t)4);
    return 0;
}

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

int vsprintf(char *buf, const char *fmt, char *args);

char pkbuf[1024];
extern tty_t default_tty;
int printk(const char *fmtstr, ...) {
    char *args = (char *)(((char *)&fmtstr) + 4);
    int size = vsprintf(pkbuf, fmtstr, args);
    tty_write(&default_tty, pkbuf, (size_t)size);
    return 0;
}

extern tty_t debug_tty;
char pdbuf[1024];
int printd(const char *fmtstr, ...) {
    char *args = (char *)(((char *)&fmtstr) + 4);
    int size = vsprintf(pdbuf, fmtstr, args);
    tty_write(&debug_tty, pdbuf, (size_t)size);
    return 0;
}

char plobuf[1024];
extern tty_t monitor_tty;
int printlo(unsigned int xpos, unsigned int ypos, const char *fmtstr, ...) {
    char *args = (char *)(((char *)&fmtstr) + 4);
    int size = vsprintf(plobuf, fmtstr, args);
    tty_write_at(&monitor_tty, xpos, ypos, plobuf, (size_t)size);
    return 0;
}

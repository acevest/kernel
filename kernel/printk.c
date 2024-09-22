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

#include <irq.h>
#include <system.h>
#include <tty.h>
int vsprintf(char *buf, const char *fmt, char *args);

void serial_write(const char *buf, size_t size);


extern tty_t *const default_tty;
int printk(const char *fmtstr, ...) {
    static char pkbuf[1024];

    ENTER_CRITICAL_ZONE(EFLAGS);

    char *args = (char *)(((char *)&fmtstr) + 4);
    int size = vsprintf(pkbuf, fmtstr, args);
    tty_write(default_tty, pkbuf, (size_t)size);
    serial_write(pkbuf, (size_t)size);

    EXIT_CRITICAL_ZONE(EFLAGS);
    return 0;
}

extern tty_t *const debug_tty;
int printd(const char *fmtstr, ...) {
    static char pdbuf[1024];
    ENTER_CRITICAL_ZONE(EFLAGS);

    char *args = (char *)(((char *)&fmtstr) + 4);
    int size = vsprintf(pdbuf, fmtstr, args);
    tty_write(debug_tty, pdbuf, (size_t)size);
    serial_write(pdbuf, (size_t)size);

    EXIT_CRITICAL_ZONE(EFLAGS);
    return 0;
}


extern tty_t *const monitor_tty;
int printlo(unsigned int xpos, unsigned int ypos, const char *fmtstr, ...) {
    static char plobuf[1024];
    char *args = (char *)(((char *)&fmtstr) + 4);
    ENTER_CRITICAL_ZONE(EFLAGS);
    int size = vsprintf(plobuf, fmtstr, args);

    tty_write_at(monitor_tty, xpos, ypos, plobuf, (size_t)size);

    EXIT_CRITICAL_ZONE(EFLAGS);
    return 0;
}

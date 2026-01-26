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

int vsprintf(char* buf, const char* fmt, char* args);

void serial_write(const char* buf, size_t size);

extern tty_t* const default_tty;

int _printk(const char* fmtstr, va_list args) {
    char* pkbuf = kmalloc(1024, 0);

    int size = vsprintf(pkbuf, fmtstr, args);

    tty_write(default_tty, pkbuf, (size_t)size);
    serial_write(pkbuf, (size_t)size);

    kfree(pkbuf);
    return 0;
}

static char _early_pkbuf[1024];
int _early_printk(const char* fmtstr, va_list args) {
    uint32_t eflags;
    irq_save(eflags);

    int size = vsprintf(_early_pkbuf, fmtstr, args);

    tty_write(default_tty, _early_pkbuf, (size_t)size);
    serial_write(_early_pkbuf, (size_t)size);

    irq_restore(eflags);
    return 0;
}

typedef int (*printk_t)(const char* fmtstr, va_list args);
static printk_t _printk_func = _early_printk;

void set_printk(printk_t pk) {
    _printk_func = pk;
}

int printk(const char* fmtstr, ...) {
    va_list args;
    va_start(args, fmtstr);

    _printk_func(fmtstr, args);

    va_end(args);
    return 0;
}

extern tty_t* const debug_tty;
int printd(const char* fmtstr, ...) {
    char* pdbuf = kmalloc(1024, 0);

    va_list args;
    va_start(args, fmtstr);
    int size = vsprintf(pdbuf, fmtstr, args);
    va_end(args);

    tty_write(debug_tty, pdbuf, (size_t)size);
    serial_write(pdbuf, (size_t)size);

    kfree(pdbuf);

    return 0;
}

extern tty_t* const monitor_tty;
int printlo(unsigned int xpos, unsigned int ypos, const char* fmtstr, ...) {
    static char plobuf[1024];

    va_list args;
    va_start(args, fmtstr);

    uint32_t eflags;
    irq_save(eflags);

    int size = vsprintf(plobuf, fmtstr, args);

    va_end(args);

    tty_write_at(monitor_tty, xpos, ypos, plobuf, (size_t)size);

    irq_restore(eflags);
    return 0;
}

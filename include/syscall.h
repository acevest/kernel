/*
 *--------------------------------------------------------------------------
 *   File Name: syscall.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Fri Jan  2 19:52:28 2009
 * Last Update: Tue Feb 23 02:32:35 2010
 * 
 *--------------------------------------------------------------------------
 */

#ifndef _SYSCALL_H
#define _SYSCALL_H

#define SYSC_NUM 256

#ifndef ASM

#include "page.h"
#include "errno.h"

int _syscall0(int nr);
int _syscall1(int nr, unsigned long a);
int _syscall2(int nr, unsigned long a, unsigned long b);
int _syscall3(int nr, unsigned long a, unsigned long b, unsigned long c);
int _syscall4(int nr, unsigned long a, unsigned long b, unsigned long c, unsigned long d);

#define syscall0(nr) _syscall0(nr)
#define syscall1(nr, a) _syscall1(nr, (unsigned long)a)
#define syscall2(nr, a, b) _syscall2(nr, (unsigned long)a, (unsigned long)b)
#define syscall3(nr, a, b, c) _syscall3(nr, (unsigned long)a, (unsigned long)b, (unsigned long)c)
#define syscall4(nr, a, b, c, d) _syscall4(nr, (unsigned long)a, (unsigned long)b, (unsigned long)c, (unsigned long)d)

enum
{
    SYSC_WRITE,
    SYSC_REBOOT,
    SYSC_FORK,
    SYSC_CLONE,
    SYSC_EXEC,
    SYSC_WAIT,
    SYSC_OPEN,
    SYSC_READ,
    SYSC_STAT,
    SYSC_EXIT,
    SYSC_PAUSE,
    SYSC_TEST,
    SYSC_DEBUG
};

#endif // ASM

#endif //_SYSCALL_H

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

#include "errno.h"
#include "page.h"

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

#endif  // ASM

#define SYSC_WRITE (0)
#define SYSC_REBOOT (1)
#define SYSC_FORK (2)
#define SYSC_CLONE (3)
#define SYSC_EXEC (4)
#define SYSC_WAIT (5)
#define SYSC_OPEN (6)
#define SYSC_READ (7)
#define SYSC_STAT (8)
#define SYSC_EXIT (9)
#define SYSC_PAUSE (10)
#define SYSC_TEST (11)
#define SYSC_DEBUG (12)
#define SYSC_BAD_NR (SYSC_NUM - 1)

#endif  //_SYSCALL_H

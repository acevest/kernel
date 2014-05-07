/*
 *--------------------------------------------------------------------------
 *   File Name: syscall.c
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Fri Jan  2 19:49:20 2009
 * Last Update: Fri Jan  2 19:49:20 2009
 * 
 *--------------------------------------------------------------------------
 */
#include "syscall.h"
#include "system.h"
#include "sched.h"
#include "msr.h"

extern void syscall_entry();
extern void init_sysc_handler_table();

unsigned long sysc_handler_table[SYSC_NUM];

void    setup_sysc()
{
    wrmsr(MSR_SYSENTER_CS,  SELECTOR_KRNL_CS,   0);
    wrmsr(MSR_SYSENTER_EIP, syscall_entry,      0);
    wrmsr(MSR_SYSENTER_ESP, &(tss.esp0),        0);

    init_sysc_handler_table();
}


int    sysc_none()
{
    int sysc_nr;
    asm("":"=a"(sysc_nr));
    printk("unsupport syscall:%d\n", sysc_nr);

    return 0;
}

int sysc_pause()
{

    return 0;
}

void    init_sysc_handler_table()
{
    int i;
    for(i=0; i<SYSC_NUM; i++)
        sysc_handler_table[i] = (unsigned long) sysc_none;

#define _sysc_(nr, sym)                                    \
    do                                                    \
    {                                                    \
        extern int sym ();                                \
        sysc_handler_table[nr] = (unsigned long) sym;    \
    }while(0);

    /* 有没有一种宏定义可以把大写直接转成小写? */
    _sysc_(SYSC_WRITE,       sysc_write);
    _sysc_(SYSC_READ_KBD,    sysc_read_kbd);
    _sysc_(SYSC_REBOOT,      sysc_reboot);
    _sysc_(SYSC_FORK,        sysc_fork);
    _sysc_(SYSC_EXEC,        sysc_exec);
    _sysc_(SYSC_OPEN,        sysc_open);
    _sysc_(SYSC_READ,        sysc_read);
    _sysc_(SYSC_STAT,        sysc_stat);
    _sysc_(SYSC_EXIT,        sysc_exit);
    _sysc_(SYSC_PAUSE,       sysc_pause);
    _sysc_(SYSC_TEST,        sysc_test);
}

int    sysc_bad_syscnr()
{
    int sysc_nr;
    asm("":"=a"(sysc_nr));
    printk("bad syscall nr:%d\n", sysc_nr);

    return 0;
}



#define SYSENTER_ASM            \
        "pushl  $1f;"           \
        "pushl    %%ecx;"       \
        "pushl    %%edx;"       \
        "pushl    %%ebp;"       \
        "movl     %%esp,%%ebp;" \
        "sysenter;"             \
        "1:"

static int __syscall0(int nr)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr));
    return __sysc_ret__;
}

static int __syscall1(int nr, unsigned long a)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr), "b"(a));
    return __sysc_ret__;
}

static int __syscall2(int nr, unsigned long a, unsigned long b)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr), "b"(a), "c"(b));
    return __sysc_ret__;
}

static int __syscall3(int nr, unsigned long a, unsigned long b, unsigned long c)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr), "b"(a), "c"(b), "d"(c));
    return __sysc_ret__;
}

static int __syscall4(int nr, unsigned long a, unsigned long b, unsigned long c, unsigned long d)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr), "b"(a), "c"(b), "d"(c), "S"(d));
    return __sysc_ret__;
}

static int __syscall5(int nr, unsigned long a, unsigned long b, unsigned long c, unsigned long d, unsigned long e)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e));
    return __sysc_ret__;
}



int _syscall0(int nr)
{
    return __syscall0(nr);
}

int _syscall1(int nr, unsigned long a)
{
    return __syscall1(nr, a);
}

int _syscall2(int nr, unsigned long a, unsigned long b)
{
    return __syscall2(nr, a, b);
}

int _syscall3(int nr, unsigned long a, unsigned long b, unsigned long c)
{
    return __syscall3(nr, a, b, c);
}

int _syscall4(int nr, unsigned long a, unsigned long b, unsigned long c, unsigned long d)
{
    return __syscall4(nr, a, b, c, d);
}

int _syscall5(int nr, unsigned long a, unsigned long b, unsigned long c, unsigned long d, unsigned long e)
{
    return __syscall5(nr, a, b, c, d, e);
}

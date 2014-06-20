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


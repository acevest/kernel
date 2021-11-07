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

#include "msr.h"
#include "sched.h"
#include "system.h"

extern void syscall_entry();
extern void init_sysc_handler_table();

unsigned long sysc_handler_table[SYSC_NUM];

void setup_sysc() {
    wrmsr(MSR_SYSENTER_CS, SELECTOR_KRNL_CS, 0);
    wrmsr(MSR_SYSENTER_EIP, syscall_entry, 0);
    // wrmsr(MSR_SYSENTER_ESP, &(tss.esp0), 0);

    init_sysc_handler_table();
}

int sysc_none() {
    int sysc_nr;
    asm("" : "=a"(sysc_nr));
    printk("unsupport syscall:%d\n", sysc_nr);

    return 0;
}

int sysc_pause(unsigned long tick) { return 0; }

int sysc_test() {
    static unsigned int cnt = 0;

    current->delay_cnt = root_task.sched_cnt % 40;

    unsigned long iflags;
    irq_save(iflags);

    current->state = TASK_WAIT;
    // 现在sysc还没有实现进程切换
    // 这个要到下一次中断之后才生效
    list_add(&(current->pend), &pend_tasks);

    irq_restore(iflags);

    schedule();

    // printl(MPL_TEST, "systest cnt %u current %08x cnt %u          ",
    //        cnt++, current, cnt);
    // printk("systest cnt %u current %08x cnt %u\n",cnt++, current, cnt);
    return 0;
}

int sysc_debug(unsigned int v) {
    static unsigned int cnt = 0;
    printl(MPL_DEBUG, "task debug syscall %u value %08x", cnt++, v);
    return 0;
}

void init_sysc_handler_table() {
    int i;
    for (i = 0; i < SYSC_NUM; i++) sysc_handler_table[i] = (unsigned long)sysc_none;

#define _sysc_(nr, sym)                              \
    do {                                             \
        extern int sym();                            \
        sysc_handler_table[nr] = (unsigned long)sym; \
    } while (0);

    _sysc_(SYSC_WRITE, sysc_write);
    _sysc_(SYSC_REBOOT, sysc_reboot);
    _sysc_(SYSC_FORK, sysc_fork);
    _sysc_(SYSC_EXEC, sysc_exec);
    _sysc_(SYSC_WAIT, sysc_wait);
    _sysc_(SYSC_OPEN, sysc_open);
    _sysc_(SYSC_READ, sysc_read);
    _sysc_(SYSC_STAT, sysc_stat);
    _sysc_(SYSC_EXIT, sysc_exit);
    _sysc_(SYSC_PAUSE, sysc_pause);
    _sysc_(SYSC_TEST, sysc_test);
    _sysc_(SYSC_DEBUG, sysc_debug);
    _sysc_(SYSC_BAD_NR, sysc_bad_nr)
}

int sysc_bad_nr() {
    int sysc_nr;

    asm("" : "=a"(sysc_nr));

    printk("bad syscall nr:%d\n", sysc_nr);

    return -1;
}

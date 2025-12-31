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
#if FIXED_SYSENTER_ESP_MODE
    wrmsr(MSR_SYSENTER_ESP, &(tss.esp0), 0);
#endif
    init_sysc_handler_table();
}

int sysc_none() {
    int sysc_nr;
    asm("" : "=a"(sysc_nr));
    printk("unsupport syscall:%d\n", sysc_nr);

    return 0;
}

extern uint64_t jiffies;

// 特别说明：如果想把这个函数的参数ticks改为int64_t
// 那么就需要在编写用户级的系统调用库函数的时候注意
// 不仅需要填写 ebx，还要填写 ecx
// 不然就可能出现诡异的一直WAIT，不会调度到该任务的问题
int sysc_wait(int ticks) {
    if (ticks < 0) {
        return -EINVAL;
    } else {
        unsigned long flags;
        irq_save(flags);
        current->state = TASK_WAIT;
        current->reason = "sysc_wait";
        current->delay_jiffies = jiffies + ticks;
        list_add(&current->pend, &delay_tasks);
        irq_restore(flags);
    }
    schedule();
    return 0;
}

int sysc_test() {
}
int sysc_pause() {
}

int sysc_debug(unsigned int v) {
    static unsigned int cnt = 0;
    printl(MPL_DEBUG, "task debug syscall %u value %08x", cnt++, v);
    return 0;
}

int sysc_rand() {
    uint32_t rand = jiffies;
    return (int)rand;
}

void init_sysc_handler_table() {
    int i;
    for (i = 0; i < SYSC_NUM; i++)
        sysc_handler_table[i] = (unsigned long)sysc_none;

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
    _sysc_(SYSC_RAND, sysc_rand);
    _sysc_(SYSC_BAD_NR, sysc_bad_nr);
}

int sysc_bad_nr() {
    int sysc_nr;

    asm("" : "=a"(sysc_nr));

    printk("bad syscall nr:%d\n", sysc_nr);

    return -1;
}

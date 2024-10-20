/*
 * ------------------------------------------------------------------------
 *   File Name: task_root.c
 *      Author: Zhao Yanbai
 *              2021-11-15 12:20:22 Monday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <disk.h>
#include <fcntl.h>
#include <ide.h>
#include <io.h>
#include <irq.h>
#include <page.h>
#include <processor.h>
#include <sched.h>
#include <stat.h>
#include <stdio.h>
#include <syscall.h>
#include <system.h>
#include <types.h>

int do_fork(pt_regs_t *regs, unsigned long flags);
int sysc_wait(int ticks);

#define get_eflags(x) __asm__ __volatile__("pushfl; popl %0;" : "=g"(x)::"memory")

void kernel_task(char *name, void *entry, void *arg) {
    pt_regs_t regs;

    memset((void *)&regs, 0, sizeof(regs));

    // 内核任务入口
    regs.edx = (unsigned long)entry;

    // 参数
    regs.ecx = (unsigned long)arg;

    regs.eax = (u32)name;

    // 创建内核任务的时候就直接指定其在fork后走的路径
    // 就不用走sysexit那个路径了
    extern void ret_from_fork_krnl();
    regs.eip = (unsigned long)ret_from_fork_krnl;
    regs.cs = SELECTOR_KRNL_CS;
    regs.ds = SELECTOR_KRNL_DS;
    regs.es = SELECTOR_KRNL_DS;
    regs.ss = SELECTOR_KRNL_DS;
    regs.fs = SELECTOR_KRNL_DS;
    regs.gs = SELECTOR_KRNL_DS;
#if 1
    get_eflags(regs.eflags);
#else
    regs.eflags = 0x200;
#endif

    int pid = do_fork(&regs, FORK_KRNL);

    printd("kernel pid %d %s\n", pid, name);
}

// 从multiboot.S进入这里
void root_task_entry() {
    printk("%08x %s %u %u\n", current, current->name, current->ticks, current->priority);

#if 0
    pt_regs_t *regs = ((pt_regs_t *)(TASK_SIZE + (unsigned long)(&root_task))) - 1;

    uint32_t *p = (uint32_t *) (TASK_SIZE + (unsigned long)(&root_task)) - 1;
    for(int i=0; i<16; i++) {
	    *p = 0x1200 | i;
	    p--;
    }
#endif

    sti();

    kernel_task("init", init_task_entry, NULL);

    strcpy(current->name, "idle");

    current->priority = 1;
    while (1) {
        asm("hlt;");
    }
}

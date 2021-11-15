/*
 * ------------------------------------------------------------------------
 *   File Name: task_root.c
 *      Author: Zhao Yanbai
 *              2021-11-15 12:20:22 Monday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <fcntl.h>
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

void disk_task_entry();
void init_task_entry();
void user_task_entry();
int do_fork(pt_regs_t *regs, unsigned long flags);

void kernel_task(char *name, void *entry) {
    pt_regs_t regs;

    memset((void *)&regs, 0, sizeof(regs));

    // 内核任务入口
    regs.edx = (unsigned long)entry;

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

    int pid = do_fork(&regs, FORK_KRNL);

    printk("kernel[%s] task pid is %d\n", name, pid);
}

// 从multiboot.S进入这里
void root_task_entry() {
    sti();

    // 有一点点垃圾事情需要处理
    // 之前内核初始化都是在关中断下进行的
    // 这就段时间有可能按键盘，然而键盘不把数据读出来就不会触发下一次中断
    // 所以得先清空一下键盘
    inb(0x60);

    kernel_task("init", init_task_entry);
    kernel_task("disk", disk_task_entry);
    kernel_task("user", user_task_entry);

    while (1) {
        asm("hlt;");
    }
}
#include <fcntl.h>
#include <init.h>
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

void root_task_entry();

TSS tss;
System system;
Desc idt[NIDT];
Desc gdt[NGDT];

char __initdata kernel_init_stack[KRNL_INIT_STACK_SIZE] __attribute__((__aligned__(PAGE_SIZE)));

int debug_wait_queue_get();

// #define __ring3section__ __attribute__((__section__(".ring3")))

// char __attribute__((__section__(".ring3.data"))) ring3_stack[PAGE_SIZE] = {0};
// void __ring3section__ ring3_entry() {
//     while (1) {
//         systest();
//     }
// }

void user_task_entry() {
    // printk("user_task_entry: %08x\n", ring3_entry);

    // 现在要准备返回用户态
    // eip --> edx
    // esp --> ecx
    // asm("sysexit;" ::"d"(ring3_entry), "c"(ring3_stack + PAGE_SIZE));
    while (1) {
        asm("hlt;");
    }
}

void init_task_entry() {
    int cnt = 0;
    pid_t id = sysc_getpid();

    // 赋予不同的优先级
    current->priority = id * 30;
    if (current->priority <= 0) {
        current->priority = 1;
    }
    if (current->priority > 100) {
        current->priority = 100;
    }

    while (1) {
        sysc_test();
        printl(MPL_TASK_1 + id - 1, "task:%d [%08x] weight %d cnt %d", id, current, current->weight, cnt++);
        // printl(MPL_TASK_1, "task:%d [%08x] weight %d cnt %d", id, current, current->weight, cnt++);
        int v = 0;  // debug_wait_queue_get();
        // printk("task:%d wait queue get %d\n", id, v);
        asm("hlt;");
    }
}

void kernel_task(char *name, void *entry) {
    pt_regs_t regs;

    memset((void *)&regs, 0, sizeof(regs));

    // 内核任务入口
    regs.edx = (unsigned long)entry;

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

void root_task_entry() {
    sti();

    kernel_task("init", init_task_entry);
    kernel_task("test", init_task_entry);
    kernel_task("user", user_task_entry);

    int cnt = 0;
    while (1) {
        sysc_test();
        printl(MPL_ROOT, "root:0 [%08x] weight %d cnt %d", current, root_task.weight, cnt++);
        // printk("root:0 [%08x] weight %d cnt %d", current, current->weight, cnt++);
        asm("sti;hlt;");
        // asm("nop;nop;nop;");
        sysc_test();
        // syscall0(SYSC_TEST);
    }
}

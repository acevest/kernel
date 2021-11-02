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

extern void *ring3;
extern void *ring3_stack_top;
void user_task_entry() {
    printk("user_task_entry: %08x %08x\n", ring3_stack_top, &ring3_stack_top);
#if 0
    asm("sti;sysexit;"::"d"(&ring3), "c"(&ring3_stack_top));
#else
    sysc_exec("/bin/shell", 0);
#endif
}

void init_task_entry() {
    int cnt = 0;
    pid_t id = sysc_getpid();

    while (1) {
        sysc_test();
        printl(MPL_TASK_1 + id - 1, "task:%d [%08x] weight %d cnt %d", id, current, current->weight, cnt++);
        // printl(MPL_TASK_1, "task:%d [%08x] weight %d cnt %d", id, current, current->weight, cnt++);
        int v = 0;  // debug_wait_queue_get();
        // printk("task:%d wait queue get %d\n", id, v);
    }
}

int kernel_fork() {
    int pid = 0;

    pt_regs_t *regs = ((pt_regs_t *)(TASK_SIZE + ((unsigned long)current))) - 1;
    unsigned long *pedx = &(regs->edx);

    asm volatile(
        "movl $1f, %[pedx];"
        "call do_fork;"
        "1:"
        : "=a"(pid)
        : [pedx] "m"(pedx));

    return pid;
}

void kernel_task(void *entry) {
    pt_regs_t regs;
    memset((void *)&regs, 0, sizeof(regs));
    regs.edx = (unsigned long)entry;
    // int pid = do_fork(&regs, FORK_KRNL);
    // int pid = kernel_fork();
    int pid = do_fork(&regs, FORK_KRNL);
    printk("kernel task pid is %d\n", pid);
    enable_irq();
}

void root_task_entry() {
    kernel_task(init_task_entry);
    // kernel_task(user_task_entry);
    // kernel_task(init_task_entry);

    int cnt = 0;
    while (1) {
        sysc_test();
        printl(MPL_ROOT, "root:0 [%08x] weight %d cnt %d", current, root_task.weight, cnt++);
        // printk("root:0 [%08x] weight %d cnt %d", current, current->weight, cnt++);
        asm("sti;hlt;");
        sysc_test();
        // syscall0(SYSC_TEST);
    }
}

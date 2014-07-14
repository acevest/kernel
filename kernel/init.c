#include <io.h>
#include <sched.h>
#include <types.h>
#include <page.h>
#include <stdio.h>
#include <system.h>
#include <syscall.h>
#include <processor.h>
#include <irq.h>
#include <fcntl.h>
#include <stat.h>
#include <init.h>


void    root_task_entry();

TSS    tss;
System    system;
Desc    idt[NIDT];
Desc    gdt[NGDT];

char __initdata kernel_init_stack[KRNL_INIT_STACK_SIZE] __attribute__ ((__aligned__(PAGE_SIZE)));

int debug_wait_queue_get();

void ring3()
{
    while(1)
    {
        systest();
    }
}
static char user_task_stack[PAGE_SIZE] __attribute__ ((__aligned__(PAGE_SIZE)));
void user_task_entry()
{
    printk("user_task_entry\n");
    asm("movl $0x23,%%eax; \
    movw %%ax,%%ds; \
    movw %%ax,%%es; \
    movw %%ax,%%fs; \
    movw %%ax,%%gs; \
    pushl $0x23; \
    pushl %%ebx; \
    pushl $0x282; \
    pushl $0x1B; \
    leal ring3,%%eax; \
    pushl %%eax; \
    iret;"::"b"(user_task_stack+PAGE_SIZE));
}

void init_task_entry()
{
    int cnt = 0;
    pid_t id = sysc_getpid();

    while(1)
    {
        printl(MPL_TASK_1, "task:%d [%08x] weight %d cnt %d", id, current, current->weight, cnt++);
        //int v = debug_wait_queue_get();
        //printk("task:%d wait queue get %d\n", id, v);
    }
}

void kernel_task(void *entry)
{
    pt_regs_t regs;
    memset((void*)&regs, 0, sizeof(regs));
    regs.edx = (unsigned long) entry;
    int pid = do_fork(&regs, FORK_KRNL);
    printk("kernel task pid is %d\n", pid);
}

void root_task_entry()
{
    //kernel_task(init_task_entry);
    kernel_task(user_task_entry);

    int cnt = 0;
    while(1)
    {
        printl(MPL_ROOT, "root:0 [%08x] weight %d cnt %d", current, root_task.weight, cnt++);
        asm("sti;hlt;");
        //sysc_test();
        //syscall0(SYSC_TEST);
    }
}

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

static unsigned int eid = 1;
void debug_sem();
int debug_wait_queue_get();
void init_task_entry()
{
    printk("%s\n", __func__);
    unsigned int id = eid++;
    int i = 0;
    while(1)
    {
        i++;
        printd(id+1, "task:%d    [%08x] cnt:%d preempt_cnt %d", id, current, i, current->preempt_cnt);
        int v = debug_wait_queue_get();
        printk("task:%d wait queue get %d\n", id, v);
    }
}


void root_task_entry()
{
    {
        pt_regs_t regs;
        memset((void*)&regs, 0, sizeof(regs));
        regs.edx = (unsigned long) init_task_entry;
        int pid = do_fork(&regs, FORK_KRNL);
        printk("a pid is %d\n", pid);
    }


    {
        pt_regs_t regs;
        memset((void*)&regs, 0, sizeof(regs));
        regs.edx = (unsigned long) init_task_entry;
        int pid = do_fork(&regs, FORK_KRNL);
        printk("b pid is %d\n", pid);
    }


    int cnt;
    while(1)
    {
        printd(1, "root_task [%08x] cnt:%d preempt_cnt %d", current, cnt++, root_task.preempt_cnt);
        printd(9, "pid %d ppid %d state %d weight %d", root_task.pid, root_task.ppid, root_task.state, root_task.weight);
        asm("sti;hlt;");
        //sysc_test();
        //syscall0(SYSC_TEST);
    }
}

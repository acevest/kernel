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
    printk("hahahha %s\n", __func__);
    unsigned int id = eid++;
    int i = 0;
    while(1)
    {
        i++;

        if(i == id*100)
        {
            printk("---READ---%d\n", id);
            debug_sem();
            printk("---END----%d\n", id);
        }
        printd(id+1, "task:%d cnt:%d", id, i);
        int v = debug_wait_queue_get();
        printk("task:%d wait queue get %d\n", id, v);
        //asm("sti;");
    }
}


void root_task_entry()
{
#if 0
    while(1) {
            asm("sti;hlt;");
    }
#endif
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
        printd(1, "root_task cnt %d", cnt++);
        asm("sti;hlt;");
        //sysc_test();
        //syscall0(SYSC_TEST);
    }
}

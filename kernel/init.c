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

static unsigned int eid = 0;
void init_task_entry()
{
    printk("hahahha %s\n", __func__);
    unsigned int id = eid++;
    while(1)
    {
        printk("%d", id);
        asm("sti;hlt;");
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


    while(1)
    {
        printk("r");
        asm("sti;hlt;");
        //sysc_test();
        //syscall0(SYSC_TEST);
    }
}

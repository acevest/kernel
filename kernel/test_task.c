/*
 *--------------------------------------------------------------------------
 *   File Name: test_task.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Tue Feb  2 20:18:05 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */


#include <stdio.h>
#include <sched.h>
#include <assert.h>
#include <system.h>
task_union *    tTasks[NR_TASKS];

#if 0
void    SetuptTasks()
{
    int i;


    for(i=0; i<NR_TASKS; i++)
    {
        tTasks[i] = NULL;
    }

    tTasks[0] = &RootTsk;
    current    = tTasks[0];
}
#endif
void    add_task(void *fun)
{
#if 0
    assert(fun != NULL);
    task_union *    tsk = NULL;
    tsk = kmalloc_old(sizeof(Task));
    if(tsk == NULL)
        panic("shit happens");

    printk("tsk:%08x\n", tsk);

    tsk->pid    = get_next_pid();
    tsk->ppid    = 0;
    init_tsk_cr3(tsk);

    pt_regs_t *    r;
    r = &tsk->regs;
    memset((void *)r, 0, sizeof(pt_regs_t));
    r->ds = r->es = r->fs = r->gs = SELECTOR_USER_DS;
    r->eip        = (unsigned long)fun;
    r->cs        = SELECTOR_USER_CS;
    r->eflags    = 0x282;
    r->esp        = (unsigned long)tsk;
    r->ss        = SELECTOR_USER_SS;

    add_tsk2list(tsk);
#endif
}

#if 0
void    add_task(void *fun)
{
    assert(fun != NULL);
    task_union *    tsk = NULL;
    int i=0;
    for(i=0; i<NR_TASKS; i++)
    {
        if(tTasks[i] == NULL)
        {
            tsk = kmalloc_old(sizeof(Task));
            if(tsk == NULL)
                panic("shit happens");
            //tTasks[i] = tsk;
            break;
        }
    }

    if(i == NR_TASKS)
        panic("tasks full");

    pt_regs_t *    r;
    r = &tsk->regs;//(pt_regs_t *)(TASK_SIZE + (unsigned long)tsk);
    //printk("Add Tsk: tsk:%08x r:%08x ", tsk, r);
    //r--;
    //printk("r:%08x sizeof regs:%x ", r, sizeof(pt_regs_t));

    memset((void *)r, 0, sizeof(pt_regs_t));
    //printk("USER CS: %x\n", SELECTOR_USER_CS);
    //printk("USER DS: %x\n", SELECTOR_USER_DS);
    r->ds = r->es = r->fs = r->gs = SELECTOR_USER_DS;
    r->eip        = (unsigned long)fun;
    r->cs        = SELECTOR_USER_CS;
    r->eflags    = 0x282;
    r->esp        = (unsigned long)tsk;
    r->ss        = SELECTOR_USER_SS;


    tTasks[i] = tsk;
}
#endif

void    delay(unsigned int d)
{
    unsigned int i;
    int n = 10000;
    for(i=0; i<d*n; i++)
            ;
}

#if 0
void    test_taskA()
{
    while(1)
    {
        printf("A");
        delay(400);
    }
}

void    test_taskB()
{
    while(1)
    {
        printf("B");
        delay(500);
    }
}
#endif

/*
 *--------------------------------------------------------------------------
 *   File Name: sched.c
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Tue Feb 10 11:53:21 2009
 * Last Update: Tue Feb 10 11:53:21 2009
 * 
 *--------------------------------------------------------------------------
 */

#include "sched.h"
#include "assert.h"



pTask        current;
Task        RootTsk __attribute__((__aligned__(PAGE_SIZE)));

task_struct* task[TASK_CNT];

#define root_task task[0]
#define first_task task[0]

pid_t    get_next_pid()
{
    static pid_t    g_pid = ROOT_TSK_PID;

    return g_pid++;
}

inline    void    load_cr3(pTask    tsk)
{
    //printk("tsk %08x cr3: %08x\n",tsk, tsk->cr3);
    asm("movl %%eax,%%cr3;"::"a"(tsk->cr3));
    //int j=10000; while(j--);
}

void    init_tsk_cr3(pTask tsk)
{
    tsk->cr3 = pa2va(get_phys_pages(1));

    if(tsk->cr3 == NULL)
        panic("failed init tsk cr3");

    memset(tsk->cr3, 0, PAGE_SIZE);
    memcpy((void *)tsk->cr3, (void*)system.page_dir, PAGE_SIZE);
    tsk->cr3 = (void *)va2pa(tsk->cr3);
}

void    init_root_tsk()
{
    int i;

    root_task->pid    = get_next_pid();
    root_task->ppid    = 0;

    for(i=0; i<NR_OPENS; i++)
        root_task->fps[i] = 0;

    /* 这个时候还没有进程开始 */
    root_task->esp0    = tss.esp0;

    init_tsk_cr3(root_task);
    load_cr3(root_task);

    current = root_task;
/*
    // 栈
    void *stack = kmalloc(PAGE_SIZE);
    if(stack == NULL)
        panic("stack");
    stack = va2pa(stack);

    printk("Stack : %08x\n", stack);

    u32 *pde = pa2va(current->cr3);
    u32 *pte = (u32 *)kmalloc(PAGE_SIZE);
    if(pte == NULL)
        panic("root task stack pte");
    pte[1023] = stack + 7;
    printk("pte: %08x\n", pte);
    pde[(KRNLADDR>>22)-1] = va2pa(pte) + 7;


    printk("CR3:%08x\n", current->cr3);
    asm("movl %%eax,%%cr3;"::"a"(current->cr3));
*/
}

void    setup_tasks()
{

    /* 初始化第一个特殊的进程 */
    init_root_tsk();
#if 0
    add_task(test_taskB);
    add_task(test_taskA);
#endif
}


task_struct *get_unused_task_pcb()
{
    unsigned int i;
    for(i=0; i<TASK_CNT; ++i)
    {

    }
}

inline    pTask get_next_tsk()
{
    static unsigned int inx = 0;
    unsigned int i = 0;
    task_struct *tsk = root_task;

    for(i=0; i<TASK_CNT; ++i)
    {
        inx = (inx + i) % TASK_CNT;

        task_struct *p = root_task + inx;

        if(tsk->state == TASK_RUNNING)
        {
            tsk = p;
            break;
        }
    }

    return tsk;
}

#if 1
inline    void set_esp0(pTask tsk)
{
    tss.esp0 = tsk->esp0;
}
inline void    switch_to()
{

    //printk("current:%08x esp0:%08x\n", current, current->esp0);
    load_cr3(current);
    set_esp0(current);
}
inline void context_switch(pTask prev, pTask next)
{
#if 1
    //pTask    last;
    unsigned long eax, ebx, ecx, edx, esi, edi;
    //asm("xchg %bx, %bx");
    asm volatile(
    "pushfl;"
    "pushl    %%ebp;"
    "movl    %%esp,%[prev_esp];"
    "movl    %[next_esp],%%esp;"
    "movl    $1f,%[prev_eip];"
    "pushl    %[next_eip];"
    "jmp    switch_to;"
    "1:"
    "popl    %%ebp;"
    "popfl;"
    :    [prev_esp] "=m"    (prev->esp),
        [prev_eip] "=m"    (prev->eip),
        "=a" (prev),    "=b" (ebx),    "=c" (ecx),
        "=d" (edx),    "=S" (esi),    "=D" (edi)
    :    [next_esp] "m"    (next->esp),
        [next_eip] "m"    (next->eip),
        [prev]    "a" (prev),
        [next]    "d" (next)
    :    "memory"
    );
#endif
}

unsigned long    schedule()
{
    pTask    tsk, prev, next;

    cli();    // For Safe.
    tsk = current;
    do
    {
        tsk = get_next_tsk(tsk);
    }while(tsk->state == TASK_EXITING); /* 简单实现 */

    if(current == tsk)
        return;

    //tsk = current;
    //printk("tsk:%08x\t", tsk);
    //current = tsk;
    prev = current;
    current = next = tsk;
    context_switch(prev, next);
}
#endif


inline void wake_up(pWaitQueue wq)
{
    
}

inline void sleep_on(pWaitQueue wq)
{

}

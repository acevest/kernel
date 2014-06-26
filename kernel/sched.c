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
#include "mm.h"
#include "init.h"

task_union root_task __attribute__((__aligned__(PAGE_SIZE)));

pid_t get_next_pid()
{
    static pid_t g_pid = ROOT_TSK_PID;

    pid_t pid = g_pid++;

    return pid;
}

inline void load_cr3(task_union *tsk)
{
    LOAD_CR3(tsk->cr3);
}

void init_tsk_cr3(task_union * tsk)
{
    tsk->cr3 = (unsigned long) pa2va(get_phys_pages(1));

    if(tsk->cr3 == 0)
        panic("failed init tsk cr3");

    memset((void *)tsk->cr3, 0, PAGE_SIZE);
    memcpy((void *)tsk->cr3, (void*)system.page_dir, PAGE_SIZE);
    tsk->cr3 = va2pa(tsk->cr3);
}

extern pde_t __initdata init_pgd[PDECNT_PER_PAGE] __attribute__((__aligned__(PAGE_SIZE)));
void init_root_tsk()
{
    int i;

    // never use memset to init root_task
    // because the stack is at top of the root_task
    // memset((char*)&root_task, 0, sizeof(root_task));

    root_task.preempt_cnt = 0;
    root_task.pid    = get_next_pid();
    root_task.ppid   = 0;
    root_task.state  = TASK_RUNNING;
    root_task.weight = TASK_INIT_WEIGHT;
    INIT_LIST_HEAD(&root_task.list);

    for(i=0; i<NR_OPENS; i++)
        root_task.fps[i] = 0;

    tss.esp0        = ((unsigned long)&root_task) + sizeof(root_task);
    root_task.esp0  = tss.esp0;
    root_task.cr3   = (unsigned long)init_pgd;

    printk("init_root_task tss.esp0 %08x\n", tss.esp0);
}

kmem_cache_t *task_union_cache;

void setup_tasks()
{

    init_root_tsk();

    task_union_cache = kmem_cache_create("task_union", sizeof(task_union), PAGE_SIZE);
    if(0 == task_union_cache)
        panic("setup tasks failed. out of memory");
}

task_union *alloc_task_union()
{
    return (task_union *) kmem_cache_alloc(task_union_cache, 0);
}


inline task_union *get_next_tsk()
{
    return 0;
}

inline void set_esp0(task_union * tsk)
{
    tss.esp0 = tsk->esp0;
}

inline void switch_to()
{
    LOAD_CR3(current->cr3);
    set_esp0(current);
}

inline void context_switch(task_union * prev, task_union * next)
{
    unsigned long eax, ebx, ecx, edx, esi, edi;
    asm volatile(
    "pushfl;"
    "pushl  %%ebp;"
    "movl   %%esp,%[prev_esp];"
    "movl   %[next_esp],%%esp;"
    "movl   $1f,%[prev_eip];"
    "pushl  %[next_eip];"
    "jmp    switch_to;"
    "1:"
    "popl   %%ebp;"
    "popfl;"
    :   [prev_esp] "=m" (prev->esp),
        [prev_eip] "=m" (prev->eip),
        "=a" (prev),"=b" (ebx), "=c" (ecx),
        "=d" (edx), "=S" (esi), "=D" (edi)
    :   [next_esp] "m"(next->esp),
        [next_eip] "m"(next->eip),
        [prev]    "a" (prev),
        [next]    "d" (next)
    :    "memory"
    );
}

unsigned long schedule()
{
    task_union *sel = &root_task;
    task_union *p = 0;
    list_head_t *pos = 0, *t=0;

    unsigned int max_weight = 0;

    unsigned long iflags;
    irq_save(iflags);
    list_for_each_safe(pos, t, &root_task.list)
    {
        p = list_entry(pos, task_union, list);

        if(p->state != TASK_RUNNING)
            continue;

        if(p->weight > max_weight)
        {
            max_weight = p->weight;
            sel = p;
        }
        else if(p->weight == 0)
        {
            p->weight = TASK_INIT_WEIGHT;
        }
    }
    irq_restore(iflags);

    sel->weight--;

    task_union *prev = current;
    task_union *next = sel;

    if(prev != sel)
    {
        //unsigned long flags;
        //irq_save(flags);
        //LOAD_CR3(root_task.cr3);
        context_switch(prev, next);
        //irq_restore(flags);
    }
}

void debug_sched()
{
    task_union *p = list_entry(current->list.next, task_union, list);
    p->state = (p->state == TASK_RUNNING) ? TASK_WAIT: TASK_RUNNING;
}

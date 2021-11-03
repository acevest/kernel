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

void load_cr3(task_union *tsk)
{
    LOAD_CR3(tsk->cr3);
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
    strcpy(root_task.name, "root_task");
    INIT_LIST_HEAD(&root_task.list);


    //  TODO
    //for(i=0; i<NR_OPENS; i++)
    //    root_task.fps[i] = 0;

    root_task.esp0  = ((unsigned long)&root_task) + sizeof(root_task);
    root_task.cr3   = (unsigned long)init_pgd;

    tss.esp0        = root_task.esp0;

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


void switch_to()
{
    LOAD_CR3(current->cr3);
    tss.esp0 = current->esp0;
}

void context_switch(task_union * prev, task_union * next)
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

task_union *find_task(pid_t pid)
{
    task_union *p = 0;
    list_head_t *pos = 0, *tmp=0;

    unsigned long iflags;
    irq_save(iflags);
    list_for_each_safe(pos, tmp, &root_task.list)
    {
        p = list_entry(pos, task_union, list);
        if(p->pid == pid)
            break;
    }
    irq_restore(iflags);

    return p;
}

static const char *task_state(unsigned int state)
{
    static const char s[][16] = {
        "  ERROR",
        "RUNNING",
        "   WAIT",
        "EXITING",
    };

    if(state >= TASK_END)
        state = TASK_UNUSED;

    return s[state];
}

unsigned long schedule()
{
    static turn = 0;
    task_union *sel = 0;
    task_union *p = 0;
    list_head_t *pos = 0, *t=0;

    unsigned long iflags;
    irq_save(iflags);
    printl(MPL_ROOT, "root:%d [%08x] cnt %u", root_task.pid, &root_task, root_task.cnt);
    list_for_each_safe(pos, t, &root_task.list)
    {
        p = list_entry(pos, task_union, list);

        printl(MPL_ROOT+p->pid, "task:%d [%08x] state %s cnt %u", p->pid, p, task_state(p->state), p->cnt);

        if(p->state != TASK_RUNNING)
        {
            continue;
        }

        printd("%08x weight %d\n", p, p->weight);


        if(sel == 0 && p->weight != turn)
        {
            p->weight = turn;
            sel = p;
        }
    }
    irq_restore(iflags);


    if(sel == 0)
    {
        sel = &root_task;
        turn ++;
    }


    task_union *prev = current;
    task_union *next = sel;

    if(prev != next)
    {
        context_switch(prev, next);
    }
}

void debug_sched()
{
    task_union *p = list_entry(current->list.next, task_union, list);
    p->state = (p->state == TASK_RUNNING) ? TASK_WAIT: TASK_RUNNING;
}

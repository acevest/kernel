/*
 *--------------------------------------------------------------------------
 *   File Name: fork.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sun Feb  7 13:25:28 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */

#include <sched.h>

int sysc_fork(pt_regs_t regs)
{
    return do_fork(&regs, 0);
}

extern void ret_from_fork_user();
extern void ret_from_fork_krnl();

int do_fork(pt_regs_t *regs, unsigned long flags)
{
    task_union *tsk;
    tsk = alloc_task_union();
    printk("fork task %08x flags %08x\n", tsk, flags);
    if(tsk == NULL)
        panic("can not malloc PCB");

    {
        tsk->cr3 = (unsigned long) alloc_one_page(0);
        if(tsk->cr3 == 0)
            panic("failed init tsk cr3");

        task_union *t = current;

        unsigned int i, j;
        pde_t *pde_src = (pde_t*) current->cr3;
        pde_t *pde_dst = (pde_t*) tsk->cr3;

        memcpy((void *)tsk->cr3, (void*)current->cr3, PAGE_SIZE);

        for(i=0; i<PAGE_PDE_CNT; ++i)
        {
            unsigned long spde = (unsigned long) pde_src[i];
            unsigned long dpde = 0;
            if(spde != 0)
                dpde = PAGE_FLAGS(spde) | (unsigned long) va2pa(alloc_one_page(0));
            pde_dst[i] = dpde;


            pte_t *pte_src = pa2va(PAGE_ALIGN(spde));
            pte_t *pte_dst = pa2va(PAGE_ALIGN(dpde));
            for(j=0; j< PAGE_PTE_CNT; ++j)
            {
                pte_src[j] &= ~PAGE_WR;
                pte_dst[j] = pte_src[j];

                page_t *page = pa2page(pte_src[j]);
                page->count ++;
            }
        }
    }

    memcpy(tsk, current, sizeof(task_union));

    tsk->pid    = get_next_pid();
    tsk->ppid   = current->pid;

    pt_regs_t *child_regs = ((pt_regs_t *) (TASK_SIZE+(unsigned long) tsk)) - 1;

    *child_regs = *regs;

    child_regs->eax = 0;
    child_regs->eflags |= 0x200; //enable IF


    tsk->esp0   = TASK_SIZE + (unsigned long) tsk;
    tsk->esp    = (unsigned long) child_regs;
    tsk->eip    = (unsigned long) ret_from_fork_user;

    if(flags & FORK_KRNL)
    {
        tsk->eip= (unsigned long) ret_from_fork_krnl;
    }

    printk("tsk %08x child_regs esp %08x esp0 %08x\n", tsk, tsk->esp, tsk->esp0);

    tsk->state = TASK_RUNNING;


    INIT_LIST_HEAD(&tsk->list);
    // TODO Lock
    list_add(&tsk->list, &root_task.list);

    return (int)tsk->pid;
}

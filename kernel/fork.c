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

#include <page.h>
#include <sched.h>

extern pid_t get_next_pid();
extern list_head_t all_tasks;

int do_fork(pt_regs_t* regs, unsigned long flags) {
    task_t* tsk;
    tsk = alloc_task_t();

    printd("fork task %08x flags %08x\n", tsk, flags);
    if (tsk == NULL) {
        panic("can not malloc PCB");
    }

    memcpy(tsk, current, sizeof(task_t));

    assert(tsk->magic == TASK_MAGIC);

    tsk->state = TASK_INITING;

    INIT_LIST_HEAD(&tsk->list);
    unsigned long iflags;
    irq_save(iflags);
    list_add(&tsk->list, &all_tasks);
    irq_restore(iflags);

    tsk->cr3 = (uint32_t)page2pa(alloc_one_page(0));
    assert(tsk->cr3 != 0);

    unsigned int i, j;
    pde_t* pde_src = (pde_t*)pa2va(current->cr3);
    pde_t* pde_dst = (pde_t*)pa2va(tsk->cr3);

    memcpy((void*)pa2va(tsk->cr3), (void*)pa2va(current->cr3), PAGE_SIZE);

    for (i = 0; i < PAGE_PDE_CNT; ++i) {
        unsigned long spde = (unsigned long)pde_src[i];
        unsigned long dpde = 0;

        if (i >= get_npde(PAGE_OFFSET)) {
            pde_dst[i] = pde_src[i];
            continue;
        }

        if (pde_src[i] == 0) {
            continue;
        }

        pde_dst[i] = pde_src[i] & (~PDE_RW);
    }

    pt_regs_t* child_regs = ((pt_regs_t*)(TASK_SIZE + (unsigned long)tsk)) - 1;

    // printd("child regs: %x %x\n", child_regs, regs);
    memcpy(child_regs, regs, sizeof(*regs));

    // child_regs->eflags |= 0x200;

    if (flags & FORK_KRNL) {
        strcpy(tsk->name, (char*)(child_regs->eax));
        child_regs->eax = 0;
    } else {
        child_regs->eip = *((unsigned long*) && fork_child);
    }
    printk("%s fork %s EFLAGS %08x\n", current->name, tsk->name, regs->eflags);

    // 这一句已经不需要了，通过fork_child已经能给子进程返回0了
    // child_regs->eax = 0;

    tsk->pid = get_next_pid();
    tsk->ppid = current->pid;
    tsk->priority = current->priority;
    tsk->ticks = tsk->priority;

    tsk->need_resched = 0;
    tsk->sched_cnt = 0;
    tsk->sched_keep_cnt = 0;

    // for switch_to
    tsk->eip = child_regs->eip;
    tsk->esp = (unsigned long)child_regs;
    tsk->esp0 = TASK_SIZE + (unsigned long)tsk;

    printd("task %08x child_regs esp %08x esp0 %08x\n", tsk, tsk->esp, tsk->esp0);

    tsk->state = TASK_READY;

    void add_task_for_monitor(task_t * tsk);
    add_task_for_monitor(tsk);

    return (int)tsk->pid;

fork_child:
    return 0;
}

int sysc_fork(pt_regs_t regs) {
    return do_fork(&regs, 0);
}

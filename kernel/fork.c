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

extern void ret_from_fork_user();
extern void ret_from_fork_krnl();
extern pid_t get_next_pid();
extern list_head_t all_tasks;

int do_fork(pt_regs_t *regs, unsigned long flags) {
    task_union *tsk;
    tsk = alloc_task_union();

    printd("fork task %08x flags %08x\n", tsk, flags);
    if (tsk == NULL) {
        panic("can not malloc PCB");
    }

    memcpy(tsk, current, sizeof(task_union));

    assert(tsk->magic == TASK_MAGIC);

    tsk->state = TASK_INITING;

    INIT_LIST_HEAD(&tsk->list);
    unsigned long iflags;
    irq_save(iflags);
    list_add(&tsk->list, &all_tasks);
    irq_restore(iflags);

    tsk->cr3 = va2pa((unsigned long)alloc_one_page(0));
    assert(tsk->cr3 != 0);

    unsigned int i, j;
    pde_t *pde_src = (pde_t *)pa2va(current->cr3);
    pde_t *pde_dst = (pde_t *)pa2va(tsk->cr3);

    memcpy((void *)pa2va(tsk->cr3), (void *)pa2va(current->cr3), PAGE_SIZE);

    for (i = 0; i < PAGE_PDE_CNT; ++i) {
        unsigned long spde = (unsigned long)pde_src[i];
        unsigned long dpde = 0;

        if (i >= 768) {
            pde_dst[i] = pde_src[i];
            continue;
        }

        if (pde_src[i] == 0) {
            continue;
        }

#if 1
        pde_dst[i] = pde_src[i] & (~PDE_RW);
#else
        // 这里不用再为每个PDE拷贝一次PageTable，只需要拷贝PageDirectory并将其低于768的写权限去掉
        // 同时需要修改缺页异常doPageFault的逻辑
        if (PAGE_ALIGN(spde) != 0) {
            dpde = alloc_one_page(0);
            assert(dpde != 0);
            memset((void *)dpde, 0, PAGE_SIZE);
            dpde = PAGE_FLAGS(spde) | (unsigned long)va2pa(dpde);
        } else {
            pde_dst[i] = 0;
            continue;
        }
        pde_dst[i] = dpde;

        pte_t *pte_src = pa2va(PAGE_ALIGN(spde));
        pte_t *pte_dst = pa2va(PAGE_ALIGN(dpde));
        for (j = 0; j < PAGE_PTE_CNT; ++j) {
            pte_src[j] &= ~PAGE_WR;
            pte_dst[j] = pte_src[j];

            if (pte_src[j] == 0) {
                continue;
            }
            printk("----pde[%u] pte_src[%u] %08x\n", i, j, pte_src[j]);
            page_t *page = pa2page(pte_src[j]);
            page->count++;
        }
#endif
    }

    pt_regs_t *child_regs = ((pt_regs_t *)(TASK_SIZE + (unsigned long)tsk)) - 1;

    // printd("child regs: %x %x\n", child_regs, regs);
    memcpy(child_regs, regs, sizeof(*regs));

    if (flags & FORK_KRNL) {
        strcpy(tsk->name, (char *)(child_regs->eax));
        child_regs->eax = 0;
    } else {
        child_regs->eip = *((unsigned long *)&&fork_child);
    }

    // 这一句已经不需要了，通过fork_child已经能给子进程返回0了
    // child_regs->eax = 0;

    tsk->pid = get_next_pid();
    tsk->ppid = current->pid;
    tsk->priority = current->priority;
    tsk->ticks = tsk->priority;
    tsk->turn = 0;  //
    tsk->need_resched = 0;
    tsk->sched_cnt = 0;
    tsk->sched_keep_cnt = 0;

    // for switch_to
    tsk->eip = child_regs->eip;
    tsk->esp = (unsigned long)child_regs;
    tsk->esp0 = TASK_SIZE + (unsigned long)tsk;

    printd("task %08x child_regs esp %08x esp0 %08x\n", tsk, tsk->esp, tsk->esp0);

    tsk->state = TASK_READY;

    return (int)tsk->pid;

fork_child:
    return 0;
}

int sysc_fork(pt_regs_t regs) { return do_fork(&regs, 0); }

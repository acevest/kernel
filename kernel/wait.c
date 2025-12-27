/*
 *--------------------------------------------------------------------------
 *   File Name: wait.c
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Mon Feb 22 20:45:22 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */
#include <sched.h>
#include <wait.h>

volatile void init_wait_queue_head(wait_queue_head_t *wqh) { INIT_LIST_HEAD(&(wqh->task_list)); }

volatile void prepare_to_wait(wait_queue_head_t *head, wait_queue_entry_t *wqe, unsigned int state) {
    unsigned long flags;
    irq_save(flags);
    // 进程可能等待一个condition满足后被唤醒
    // 然后又发现这个conditon又不满足了，需要继续wait
    // 这时候又会把自己加到等待队列里
    // 所以这里需要加一个判断，如果已经加过了，就不需要再加了，不然会出问题
    if (list_empty(&wqe->entry)) {
        list_add_tail(&wqe->entry, &head->task_list);
    }
    set_current_state(state);
    current->reason = "p_wait";
    irq_restore(flags);
}

volatile void __end_wait(wait_queue_entry_t *wqe) {
    set_current_state(TASK_READY);
    current->reason = "e_wait";
    unsigned long flags;
    irq_save(flags);
    list_del_init(&wqe->entry);
    irq_restore(flags);
}

volatile void add_wait_queue(wait_queue_head_t *head, wait_queue_entry_t *wqe) {
    unsigned long flags;
    irq_save(flags);
    list_add_tail(&wqe->entry, &head->task_list);
    irq_restore(flags);
}

volatile void del_wait_queue(wait_queue_head_t *head, wait_queue_entry_t *wqe) {
    unsigned long flags;
    irq_save(flags);
    list_del(&wqe->entry);
    irq_restore(flags);
}

volatile void __wake_up(wait_queue_head_t *head, int nr) {
    unsigned long flags;
    wait_queue_entry_t *p, *tmp;
    irq_save(flags);
    list_for_each_entry_safe(p, tmp, &head->task_list, entry) {
        assert(p->task != NULL);
        // printk("wakeup %s\n", p->task->name);
        p->task->state = TASK_READY;
        p->task->reason = "wake_up";

        list_del_init(&p->entry);

        --nr;
        if (nr == 0) {
            break;
        }
    }
    irq_restore(flags);

    // no schedule() here.
}

volatile void wake_up(wait_queue_head_t *head) {
    //
    __wake_up(head, 0);  // wake up all
}

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

volatile void init_wait_queue_head(wait_queue_head_t *wqh) { INIT_LIST_HEAD(&wqh->task_list); }

volatile void prepare_to_wait(wait_queue_head_t *head, wait_queue_t *wq, unsigned int state) {
    unsigned long flags;
    irq_save(flags);
    if (list_empty(&wq->task_list)) {
        list_add_tail(&wq->task_list, &head->task_list);
    }
    set_current_state(state);
    irq_restore(flags);
}

volatile void __end_wait(wait_queue_t *wq) {
    set_current_state(TASK_READY);
    unsigned long flags;
    irq_save(flags);
    list_del_init(&wq->task_list);
    irq_restore(flags);
}

volatile void add_wait_queue(wait_queue_head_t *head, wait_queue_t *wq) {
    unsigned long flags;
    irq_save(flags);
    list_add_tail(&wq->task_list, &head->task_list);
    irq_restore(flags);
}

volatile void del_wait_queue(wait_queue_head_t *head, wait_queue_t *wq) {
    unsigned long flags;
    irq_save(flags);
    list_del(&wq->task_list);
    irq_restore(flags);
}

volatile void __wake_up(wait_queue_head_t *head, int nr) {
    unsigned long flags;
    wait_queue_t *p, *tmp;
    irq_save(flags);
    list_for_each_entry_safe(p, tmp, &head->task_list, task_list) {
        p->task->state = TASK_READY;
        current->reason = "wake_up";

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
    __wake_up(head, 1);
}

volatile void wait_on(wait_queue_head_t *head) {
    DECLARE_WAIT_QUEUE(wait, current);

    unsigned long flags;
    irq_save(flags);

    current->state = TASK_WAIT;
    current->reason = "sleep_on";

    list_add_tail(&wait.task_list, &head->task_list);

    irq_restore(flags);

    schedule();

    __end_wait(&wait);
}

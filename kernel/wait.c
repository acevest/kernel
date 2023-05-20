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

volatile void prepare_to_wait(wait_queue_head_t *head, wait_queue_t *wq, unsigned int state) {
    unsigned long flags;
    irq_save(flags);
    if (list_empty(&wq->task_list)) {
        list_add_tail(&wq->task_list, &head->task_list);
    }
    set_current_state(state);
    irq_restore(flags);
}

volatile void __end_wait(wait_queue_head_t *head, wait_queue_t *wq) {
    set_current_state(TASK_READY);
    unsigned long flags;
    irq_save(flags);
    list_del_init(&wq->task_list);
    irq_restore(flags);
}

volatile void sleep_on(wait_queue_head_t *head) {
    DECLARE_WAIT_QUEUE(wait, current);

    unsigned long flags;
    irq_save(flags);

    current->state = TASK_WAIT;

    list_add_tail(&wait.task_list, &head->task_list);

    irq_restore(flags);

    schedule();

    // wake_up操作会把wait从heat链表上删除
    // 所以这里就不用做什么了
}

volatile void __wake_up(wait_queue_head_t *head, int nr) {
    unsigned long flags;
    wait_queue_t *p, *tmp;
    irq_save(flags);
    list_for_each_entry_safe(p, tmp, &head->task_list, task_list) {
        list_del(&p->task_list);
        // printk("wakeup: %s\n", p->task->name);
        p->task->state = TASK_READY;

        --nr;
        if (nr == 0) {
            break;
        }
    }
    irq_restore(flags);

    // no schedule() here.
}

volatile void wake_up(wait_queue_head_t *head) { __wake_up(head, 1); }

#include <irq.h>
DECLARE_WAIT_QUEUE_HEAD(debug_wq);
unsigned int debug_global_var = 0;
int debug_wait_queue_get() {
    unsigned int v = 0;
    task_union *task = current;
    DECLARE_WAIT_QUEUE(wait, task);
    add_wait_queue(&debug_wq, &wait);

    while (1) {
        printd("pid %d is going to wait\n", sysc_getpid());
        task->state = TASK_WAIT;

        disable_irq();
        v = debug_global_var;
        if (debug_global_var != 0) debug_global_var--;
        enable_irq();

        if (v != 0) break;

        schedule();
        printd("pid %d is running\n", sysc_getpid());
    }

    printd("pid %d is really running\n", sysc_getpid());
    task->state = TASK_READY;
    del_wait_queue(&debug_wq, &wait);

    return v;
}

int debug_wait_queue_put(unsigned int v) {
    debug_global_var = v;
    wake_up(&debug_wq);
}

DECLARE_WAIT_QUEUE_HEAD(sysc_wait_queue_head);

#if 1
extern unsigned int jiffies;
int sysc_wait(unsigned long cnt) {
    unsigned long flags;
    irq_save(flags);
    current->state = TASK_WAIT;
    current->delay_cnt = jiffies + cnt;
    irq_restore(flags);

    schedule();
}
#else
int sysc_wait(unsigned long pid) {
    task_union *p = find_task(pid);

    if (p == 0) {
        return 0;
    }

    if (p->state == TASK_EXITING) {
        return 0;
    }

    task_union *task = current;
    DECLARE_WAIT_QUEUE(wait, task);
    add_wait_queue(&p->wait, &wait);

    while (true) {
        // task->state = TASK_WAIT;

        unsigned long flags;
        irq_save(flags);
        unsigned int state = p->state;
        irq_restore(flags);

        if (state == TASK_EXITING)  // no need irq_save
            break;

        schedule();
    }

    task->state = TASK_READY;
    del_wait_queue(&p->wait, &wait);

    return 0;
}
#endif

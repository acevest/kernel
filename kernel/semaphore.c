/*
 * ------------------------------------------------------------------------
 *   File Name: semaphore.c
 *      Author: Zhao Yanbai
 *              Sun Jun 22 13:57:18 2014
 * Description: none
 * ------------------------------------------------------------------------
 */
#include <irq.h>
#include <sched.h>
#include <semaphore.h>

void semaphore_init(semaphore_t *s, unsigned int v) {
    s->cnt = v;
    INIT_LIST_HEAD(&(s->wait_list));
}

#if 1
volatile void down(semaphore_t *s) {
    unsigned long iflags;
    irq_save(iflags);

    if (likely(s->cnt > 0)) {
        s->cnt--;
    } else {
        task_union *task = current;
        list_add_tail(&task->wait, &s->wait_list);

        task->state = TASK_WAIT;
        task->reason = "down";

        schedule();
    }

    irq_restore(iflags);
}

// volatile bool try_down(semaphore_t *s) {
//     unsigned long iflags;
//     irq_save(iflags);

//     // if(s->cnt )

//     irq_restore(iflags);
// }

volatile void up(semaphore_t *s) {
    unsigned long iflags;
    irq_save(iflags);

    if (list_empty(&s->wait_list)) {
        s->cnt++;
    } else {
        task_union *task = list_first_entry(&s->wait_list, task_union, wait);
        list_del(&task->wait);
        task->state = TASK_READY;
        task->reason = "up";

        // 按理这里应该调用schedule再重新调度一次
        // 原因是有可能多个任务都在一个循环里争抢一个锁
        // 如果这里不让当前任务尝试放弃CPU重新调度，则在下一轮循环中它又可能抢到锁
        // 这种情况如果一直发生，就可能导致其它任务一直抢不到
        //
        // 但这里为什么又没调用schedule呢？
        // 这是因为在目前在ide_irq_bh_handler里调用了up来唤醒磁盘任务
        // 而ide_irq_bh_handler又是中断的底半处理，不应该切换任务
        // 否则会引起irq里的reenter问题，导致不能再进底半处理，也无法切换任务
        // 这里暂时先保持这样，后续再来解决这里
        // schedule();
    }

    irq_restore(iflags);
}

#else

typedef struct semaphore_waiter {
    list_head_t list;
    task_union *task;
    int up;
} semaphore_waiter_t;

#define SEMAPHORE_WAITER_INITIALIZER(name, task) \
    { .list = LIST_HEAD_INIT((name).list), .task = task, .up = 0 }

#define DECLARE_SEMAPHORE_WAITER(name, task) semaphore_waiter_t name = SEMAPHORE_WAITER_INITIALIZER(name, task)

volatile void __down(semaphore_t *s) {
    task_union *task = current;
    DECLARE_SEMAPHORE_WAITER(waiter, task);
    list_add_tail(&waiter.list, &s->wait_list);

    while (true) {
        task->state = TASK_WAIT;
        task->reason = "down";
        schedule();

        assert(waiter.up == 1);
        if (waiter.up) {
            break;
        }
    }
}

volatile void down(semaphore_t *s) {
    unsigned long iflags;
    irq_save(iflags);

    if (likely(s->cnt > 0)) {
        s->cnt--;
    } else {
        __down(s);
    }

    irq_restore(iflags);
}

volatile void __up(semaphore_t *s) {
    semaphore_waiter_t *waiter = list_first_entry(&s->wait_list, semaphore_waiter_t, list);
    list_del(&waiter->list);
    waiter->up = 1;

    waiter->task->state = TASK_READY;
    waiter->task->reason = "up";

    // 按理这里应该调用schedule再重新调度一次
    // 原因是有可能多个任务都在一个循环里争抢一个锁
    // 如果这里不让当前任务尝试放弃CPU重新调度，则在下一轮循环中它又可能抢到锁
    // 这种情况如果一直发生，就可能导致其它任务一直抢不到
    //
    // 但这里为什么又没调用schedule呢？
    // 这是因为在目前在ide_irq_bh_handler里调用了up来唤醒磁盘任务
    // 而ide_irq_bh_handler又是中断的底半处理，不应该切换任务
    // 否则会引起irq里的reenter问题，导致不能再进底半处理，也无法切换任务
    // 这里暂时先保持这样，后续再来解决这里
    // schedule();
}

volatile void up(semaphore_t *s) {
    unsigned long iflags;
    irq_save(iflags);

    // if (likely(list_empty(&s->wait_list))) {
    if (list_empty(&s->wait_list)) {
        s->cnt++;
    } else {
        __up(s);
    }

    irq_restore(iflags);
}
#endif

void mutex_lock(semaphore_t *s) { down(s); }
void mutex_unlock(semaphore_t *s) { up(s); }

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

typedef struct semaphore_waiter {
    list_head_t list;
    task_union *task;
    int up;
} semaphore_waiter_t;

#define SEMAPHORE_WAITER_INITIALIZER(name, task) \
    { .list = LIST_HEAD_INIT((name).list), .task = task, .up = 0 }

#define DECLARE_SEMAPHORE_WAITER(name, task) semaphore_waiter_t name = SEMAPHORE_WAITER_INITIALIZER(name, task)

void semaphore_init(semaphore_t *s, unsigned int v) {
    s->cnt = v;
    INIT_LIST_HEAD(&(s->wait_list));
}

volatile void __down(semaphore_t *s) {
    task_union *task = current;
    DECLARE_SEMAPHORE_WAITER(waiter, task);
    list_add_tail(&waiter.list, &s->wait_list);

    while (true) {
        task->state = TASK_WAIT;
        task->reason = "down";
        schedule();

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

void mutex_lock(semaphore_t *s) { down(s); }
void mutex_unlock(semaphore_t *s) { up(s); }

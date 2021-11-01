/*
 * ------------------------------------------------------------------------
 *   File Name: semaphore.c
 *      Author: Zhao Yanbai
 *              Sun Jun 22 13:57:18 2014
 * Description: none
 * ------------------------------------------------------------------------
 */
#include <semaphore.h>
#include <irq.h>

typedef struct semaphore_waiter
{
    list_head_t list;
    task_union *task;
    int up;
} semaphore_waiter_t;

#define SEMAPHORE_WAITER_INITIALIZER(name, task) \
    {                                            \
        .list = LIST_HEAD_INIT((name).list),     \
        .task = task,                            \
        .up = 0                                  \
    }

#define DECLARE_SEMAPHORE_WAITER(name, task) \
    semaphore_waiter_t name = SEMAPHORE_WAITER_INITIALIZER(name, task)

void __down(semaphore_t *s)
{
    task_union *task = current;
    DECLARE_SEMAPHORE_WAITER(waiter, task);
    list_add_tail(&waiter.list, &s->wait_list);

    //while(true)
    {
        task->state = TASK_WAIT;

        enable_irq();
        schedule();
        disable_irq();

        if (waiter.up)
            ; //break;
    }
}

void down(semaphore_t *s)
{
    unsigned long iflags;

    irq_save(iflags);

    if (likely(s->cnt > 0))
    {
        s->cnt--;
    }
    else
    {
        __down(s);
    }

    irq_restore(iflags);
}

void __up(semaphore_t *s)
{
    semaphore_waiter_t *waiter = list_first_entry(&s->wait_list, semaphore_waiter_t, list);
    list_del(&waiter->list);
    waiter->up = 1;

    waiter->task->state = TASK_RUNNING;
}

void up(semaphore_t *s)
{
    unsigned long iflags;

    irq_save(iflags);
    if (likely(list_empty(&s->wait_list)))
    {
        s->cnt++;
    }
    else
    {
        __up(s);
    }

    irq_restore(iflags);
}

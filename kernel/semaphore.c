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
    task_t *task;

    // 在Linux内核中这个结构体有一个up字段，这个字段的作用是防止进程被错误地唤醒
    // 例如
    // 如果一个进程在等待信号量时收到了一个信号（signal），它可能会被内核唤醒以处理这个信号。
    // 但是，如果这个进程没有真正获得信号量（即up字段为false）它应该继续等待信号量而不是继续执行
    // 本内核暂时还用不到这个字段

} semaphore_waiter_t;

void semaphore_init(semaphore_t *s, unsigned int v) {
    s->cnt = v;
    INIT_LIST_HEAD(&(s->wait_list));
}

volatile void down(semaphore_t *s) {
    unsigned long iflags;
    irq_save(iflags);

    if (likely(s->cnt > 0)) {
        s->cnt--;
        irq_restore(iflags);
    } else {
        task_t *task = current;
        semaphore_waiter_t waiter;
        waiter.task = task;
        INIT_LIST_HEAD(&waiter.list);
        list_add(&waiter.list, &s->wait_list);

        task->state = TASK_WAIT;
        task->reason = "down";

        irq_restore(iflags);

        schedule();
    }
}

volatile void up(semaphore_t *s) {
    unsigned long iflags;
    irq_save(iflags);

    if (list_empty(&s->wait_list)) {
        s->cnt++;
        irq_restore(iflags);
    } else {
        semaphore_waiter_t *waiter = list_first_entry(&s->wait_list, semaphore_waiter_t, list);
        assert(waiter != 0);
        list_del(&waiter->list);
        task_t *task = waiter->task;

        task->state = TASK_READY;
        task->reason = "up";

        irq_restore(iflags);

        // 之前把这里的schedule()删除了，是因为up()被用到了 ide_irq_bh_handler 里去唤醒磁盘任务
        // 但在中断的底半处理，不应该切换任务，因为会引起irq里的reenter问题，导致不能再进底半处理，也无法切换任务
        // 但这种方法是不对的
        // 后来就改用完成量来通知磁盘任务
        // 因此信号量这里就可以加回schedule()了
        schedule();
    }
}

void mutex_lock(semaphore_t *s) { down(s); }
void mutex_unlock(semaphore_t *s) { up(s); }

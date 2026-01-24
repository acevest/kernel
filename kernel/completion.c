/*
 * ------------------------------------------------------------------------
 *   File Name: completion.c
 *      Author: Zhao Yanbai
 *              2021-11-27 10:58:46 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <completion.h>
#include <sched.h>

void wait_completion(completion_t* x) {
    uint32_t eflags;
    DECLARE_WAIT_QUEUE_ENTRY(__wait, current);

    while (1) {
        irq_save(eflags);
        if (x->done > 0) {
            x->done--;
            irq_restore(eflags);
            return;
        }

        __prepare_to_wait(&x->wait, &__wait, TASK_WAIT);
        irq_restore(eflags);

        schedule();

        irq_save(eflags);
        __end_wait(&__wait);
        irq_restore(eflags);
    }
}

void complete(completion_t* x) {
    uint32_t iflags;
    irq_save(iflags);
    x->done++;
    wake_up_all(&x->wait);
    irq_restore(iflags);
}

void init_completion(completion_t* x) {
    x->done = 0;
    init_wait_queue_head(&x->wait);

    // 测试字段
    x->name = 0;
}

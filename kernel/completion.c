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
    wait_event(&x->wait, (x->done != 0));
    x->done--;
}

void complete(completion_t* x) {
    uint32_t iflags;
    irq_save(iflags);
    x->done++;
    __wake_up(&x->wait, 1);
    irq_restore(iflags);
}

void init_completion(completion_t* x) {
    x->done = 0;
    init_wait_queue_head(&x->wait);

    // 测试字段
    x->name = 0;
}

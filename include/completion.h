/*
 * ------------------------------------------------------------------------
 *   File Name: completion.h
 *      Author: Zhao Yanbai
 *              2021-11-27 10:58:33 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <wait.h>

typedef struct completion {
    volatile int done;
    wait_queue_head_t wait;

    // 仅用于调试
    char *name;
} completion_t;

#define COMPLETION_INITIALIZER(x) \
    { 0, WAIT_QUEUE_HEAD_INITIALIZER(x).wait }

void init_completion(completion_t *x);

void wait_completion(completion_t *x);

// 一次只唤醒一个进程
void complete(completion_t *x);

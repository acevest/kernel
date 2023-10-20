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
    int done;
    wait_queue_head_t wait;
} completion_t;

#define COMPLETION_INITIALIZER(x) \
    { 0, WAIT_QUEUE_HEAD_INITIALIZER(x).wait }

void wait_completion(completion_t *x);
void init_completion(completion_t *x);

void complete(completion_t *x);

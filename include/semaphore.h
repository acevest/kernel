/*
 * ------------------------------------------------------------------------
 *   File Name: semaphore.h
 *      Author: Zhao Yanbai
 *              Sun Jun 22 13:53:18 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <irq.h>
#include <list.h>
#include <task.h>

typedef struct semaphore {
    volatile unsigned int cnt;
    list_head_t wait_list;
} semaphore_t;

#define SEMAPHORE_INITIALIZER(name, n) \
    { .cnt = (n), .wait_list = LIST_HEAD_INIT((name).wait_list) }

void semaphore_init(semaphore_t *s, unsigned int v);

// down
// 如果s->cnt > 0不会立即重新调度进程
// 如果s->cnt == 0 会重新调度进程
volatile void down(semaphore_t *s);

// up
// 只会唤醒进程，但不会立即重新调度进程
volatile void up(semaphore_t *s);

typedef semaphore_t mutex_t;

#define MUTEX_INITIALIZER(name) \
    { .cnt = (1), .wait_list = LIST_HEAD_INIT((name).wait_list) }

#define DECLARE_MUTEX(name) mutex_t name = MUTEX_INITIALIZER(name)

#define INIT_MUTEX(ptr)                      \
    do {                                     \
        (ptr)->cnt = 1;                      \
        INIT_LIST_HEAD(&((ptr)->wait_list)); \
    } while (0)
void mutex_lock(mutex_t *);
void mutex_unlock(mutex_t *);

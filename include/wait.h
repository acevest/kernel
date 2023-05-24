/*
 *--------------------------------------------------------------------------
 *   File Name: wait.h
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Mon Feb 22 20:50:56 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */

#pragma once

#include <irq.h>
#include <list.h>

union task_union;

typedef struct wait_queue_head {
    list_head_t task_list;
} wait_queue_head_t;

typedef struct {
    union task_union *task;
    list_head_t task_list;
} wait_queue_t;

#define WAIT_QUEUE_HEAD_INITIALIZER(name) \
    { .task_list = LIST_HEAD_INIT((name).task_list) }

#define DECLARE_WAIT_QUEUE_HEAD(name) wait_queue_head_t name = WAIT_QUEUE_HEAD_INITIALIZER(name)

#define WAIT_QUEUE_INITIALIZER(name, tsk) \
    { .task = tsk, .task_list = LIST_HEAD_INIT((name).task_list) }

#define DECLARE_WAIT_QUEUE(name, tsk) wait_queue_t name = WAIT_QUEUE_INITIALIZER(name, tsk)

void init_wait_queue_head(wait_queue_head_t *wqh);
void add_wait_queue(wait_queue_head_t *head, wait_queue_t *wq);
void del_wait_queue(wait_queue_head_t *head, wait_queue_t *wq);

// prepare_to_wait 不会调用schedule
void prepare_to_wait(wait_queue_head_t *head, wait_queue_t *wq, unsigned int state);

void __end_wait(wait_queue_head_t *head, wait_queue_t *wq);

void sleep_on(wait_queue_head_t *head);
void wake_up(wait_queue_head_t *head);

void schedule();
#define __wait_event(head, condition)                  \
    do {                                               \
        DECLARE_WAIT_QUEUE(__wait, current);           \
        while (1) {                                    \
            prepare_to_wait(head, &__wait, TASK_WAIT); \
            if ((condition)) {                         \
                break;                                 \
            }                                          \
            schedule();                                \
        }                                              \
        __end_wait(head, &__wait);                     \
    } while (0)

#define wait_event(head, condition)          \
    do {                                     \
        if (!(condition)) {                  \
            __wait_event(head, (condition)); \
        }                                    \
    } while (0)

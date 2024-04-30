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
#include <sched.h>

typedef struct wait_queue_head {
    list_head_t task_list;
} wait_queue_head_t;

typedef struct {
    task_t *task;
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

void __end_wait(wait_queue_t *wq);

// 使用这个函数的时候需要注意
// 设置condition为真的语句和wake_up原子地执行
void wake_up(wait_queue_head_t *head);
void __wake_up(wait_queue_head_t *head, int nr);

// 只要保证condition设置为真时，它是和wake_up一起原子执行的
// 那就能保证condition为真是如下__wait_event和wait_event里的if不出问题
// 也就是不会出现if-break后进程又再次因为其它原因阻塞后，又被wake_up的逻辑
// 设置为READY状态
#define __wait_event(head, condition)                  \
    do {                                               \
        DECLARE_WAIT_QUEUE(__wait, current);           \
        while (1) {                                    \
            prepare_to_wait(head, &__wait, TASK_WAIT); \
            if ((condition)) {                         \
                break;                                 \
            }                                          \
            void schedule();                           \
            schedule();                                \
        }                                              \
        __end_wait(&__wait);                           \
    } while (0)

#define wait_event(head, condition)          \
    do {                                     \
        if (!(condition)) {                  \
            __wait_event(head, (condition)); \
        }                                    \
    } while (0)

// 无条件wait
void wait_on(wait_queue_head_t *head);

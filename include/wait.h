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
    task_t* task;
    list_head_t entry;
} wait_queue_entry_t;

#define WAIT_QUEUE_HEAD_INITIALIZER(name) {.task_list = LIST_HEAD_INIT((name).task_list)}

#define DECLARE_WAIT_QUEUE_HEAD(name) wait_queue_head_t name = WAIT_QUEUE_HEAD_INITIALIZER(name)

#define WAIT_QUEUE_ENTRY_INITIALIZER(name, tsk) {.task = tsk, .entry = LIST_HEAD_INIT((name).entry)}

#define DECLARE_WAIT_QUEUE_ENTRY(name, tsk) wait_queue_entry_t name = WAIT_QUEUE_ENTRY_INITIALIZER(name, tsk)

void init_wait_queue_head(wait_queue_head_t* wqh);
void add_wait_queue(wait_queue_head_t* head, wait_queue_entry_t* wq);
void del_wait_queue(wait_queue_head_t* head, wait_queue_entry_t* wq);

// prepare_to_wait 不会调用schedule
void __prepare_to_wait(wait_queue_head_t* head, wait_queue_entry_t* wqe, unsigned int state);
void prepare_to_wait(wait_queue_head_t* head, wait_queue_entry_t* wq, unsigned int state);

void __end_wait(wait_queue_entry_t* wq);

// 使用这个函数的时候需要注意
// 设置condition为真的语句和wake_up原子地执行
// wake_up只唤醒不立即重新调度
void wake_up(wait_queue_head_t* head);

void wake_up_all(wait_queue_head_t* head);

// nr == 0 代表唤醒所有
void __wake_up(wait_queue_head_t* head, int nr);

// 以下wait_event被定义成宏，而不是函数是因为condition
// condition可能是一个表达式，如果定义成函数，往下传递就直接变成了一个值
// 如果在某一步这个值变了，并不会反应在这些逻辑判断上
#define __wait_event(head, condition)                  \
    do {                                               \
        DECLARE_WAIT_QUEUE_ENTRY(__wait, current);     \
        unsigned long flags;                           \
        while (1) {                                    \
            irq_save(flags);                           \
            prepare_to_wait(head, &__wait, TASK_WAIT); \
            if ((condition)) {                         \
                __end_wait(&__wait);                   \
                irq_restore(flags);                    \
                break;                                 \
            } else {                                   \
                irq_restore(flags);                    \
                void schedule();                       \
                schedule();                            \
                irq_save(flags);                       \
                __end_wait(&__wait);                   \
                irq_restore(flags);                    \
            }                                          \
        }                                              \
    } while (0)

#define wait_event(head, condition)      \
    do {                                 \
        if ((condition)) {               \
            break;                       \
        }                                \
        __wait_event(head, (condition)); \
    } while (0)

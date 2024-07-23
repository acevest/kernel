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
    list_head_t entry;
} wait_queue_entry_t;

#define WAIT_QUEUE_HEAD_INITIALIZER(name) \
    { .task_list = LIST_HEAD_INIT((name).task_list) }

#define DECLARE_WAIT_QUEUE_HEAD(name) wait_queue_head_t name = WAIT_QUEUE_HEAD_INITIALIZER(name)

#define WAIT_QUEUE_ENTRY_INITIALIZER(name, tsk) \
    { .task = tsk, .entry = LIST_HEAD_INIT((name).entry) }

#define DECLARE_WAIT_QUEUE_ENTRY(name, tsk) wait_queue_entry_t name = WAIT_QUEUE_ENTRY_INITIALIZER(name, tsk)

void init_wait_queue_head(wait_queue_head_t *wqh);
void add_wait_queue(wait_queue_head_t *head, wait_queue_entry_t *wq);
void del_wait_queue(wait_queue_head_t *head, wait_queue_entry_t *wq);

// prepare_to_wait 不会调用schedule
void prepare_to_wait(wait_queue_head_t *head, wait_queue_entry_t *wq, unsigned int state);

void __end_wait(wait_queue_entry_t *wq);

// 使用这个函数的时候需要注意
// 设置condition为真的语句和wake_up原子地执行
// wake_up只唤醒不立即重新调度
void wake_up(wait_queue_head_t *head);

// nr == 0 代表唤醒所有
void __wake_up(wait_queue_head_t *head, int nr);

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
                __end_wait(&__wait);                   \
            }                                          \
        }                                              \
    } while (0)

#if 0
// __wait_event这个逻辑参考自Linux内核，但是发现它有BUG
// 第一个问题
// 1. 在进入到__wait_event后，prepare_to_wait前其它进程通过wake_up唤醒当前进程
// 2. prepare_to_wait把当前进程设置为TASK_WAIT
// 3. prepare_to_wait最后一句irq_restore(flags);执行完后 其实 condition 已经为true了
// 4. 从if((conditon))到break再到__end_wait前都有可能被中断打断并重新schedule()
//    只要在__end_wait的irq_save(flags);完全关闭中断前被中断打断并schedule把当前进程换下CPU，那么这次唤醒就会丢失
//    如果只有一次唤醒，那么这个唤醒丢失后，当前进程就会一直处于WAIT状态并且永远不会再被调度到
//
// 但按理Linux内核不应该会出现这种BUG，还没研究明白，先把这段代码放在这里
#define __wait_event(head, condition)                  \
    do {                                               \
        DECLARE_WAIT_QUEUE_ENTRY(__wait, current);     \
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
#endif

#define wait_event(head, condition)      \
    do {                                 \
        if ((condition)) {               \
            break;                       \
        }                                \
        __wait_event(head, (condition)); \
    } while (0)

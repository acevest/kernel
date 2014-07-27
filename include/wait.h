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

#include <list.h>
#include <irq.h>
#include <task.h>

typedef struct
{
    task_union *task;
    list_head_t task_list;
} wait_queue_t;

#define WAIT_QUEUE_HEAD_INITIALIZER(name)           \
{                                                   \
    .task_list  = LIST_HEAD_INIT((name).task_list)  \
}

#define DECLARE_WAIT_QUEUE_HEAD(name)               \
    wait_queue_head_t name = WAIT_QUEUE_HEAD_INITIALIZER(name)

#define WAIT_QUEUE_INITIALIZER(name, tsk)           \
{                                                   \
    .task       = tsk,                              \
    .task_list  = LIST_HEAD_INIT((name).task_list)  \
}

#define DECLARE_WAIT_QUEUE(name, tsk)               \
    wait_queue_t name = WAIT_QUEUE_INITIALIZER(name, tsk)


void init_wait_queue(wait_queue_head_t * wqh);
void add_wait_queue(wait_queue_head_t *wqh, wait_queue_t *wq);
void del_wait_queue(wait_queue_head_t *wqh, wait_queue_t *wq);

/*
 *--------------------------------------------------------------------------
 *   File Name: sched.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Sat Feb  7 21:43:49 2009
 * Last Update: Sat Feb  7 21:43:49 2009
 *
 *--------------------------------------------------------------------------
 */

#pragma once

#include <task.h>
#include <wait.h>

#define FORK_USER 0
#define FORK_KRNL 1

unsigned long schedule();

void wake_up(wait_queue_head_t *wqh);

extern task_union root_task;

extern void load_cr3(task_union *tsk);

extern list_head_t all_tasks;
extern list_head_t delay_tasks;
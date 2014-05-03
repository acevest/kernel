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

#ifndef    _SCHED_H
#define _SCHED_H

#include <task.h>
#define NR_TASKS    3
//task_union *    tTasks[NR_TASKS];
//void    add_task();
//void    SetupTasks();
//void    test_taskA();
//void    test_taskB();
//unsigned long schedule(pt_regs_t *    regs);
unsigned long schedule();

pid_t    get_next_pid();
void    init_tsk_cr3(task_union *);


inline void wake_up(pWaitQueue wq);
inline void sleep_on(pWaitQueue wq);

#define TASK_CNT 64

#endif //_SCHED_H

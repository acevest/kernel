/*
 *--------------------------------------------------------------------------
 *   File Name: task.h
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Thu Dec 31 16:54:48 2009
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */

#ifndef    _TASK_H
#define _TASK_H

#include <page.h>
#include <list.h>
#include <types.h>
#include <processor.h>
#include <system.h>
#include <wait.h>
#include <fs.h>
#define TASK_PAGES    (2)
#define TASK_SIZE    (TASK_PAGES<<PAGE_SHIFT)

enum
{
    TASK_UNUSED,
    TASK_RUNNING,
    TASK_UNINTERRUPTIBLE,
    TASK_INTERRUPTIBLE,
    TASK_EXITING
};

typedef union task_union
{
    struct
    {
        pt_regs_t        regs;

        unsigned long    esp0;    /* 指示发生在用户态的中断在进入
                       内核态后的栈位置 */

        /* 进程切换时用 */
        unsigned long    esp;
        unsigned long    eip;

        pid_t        pid;
        pid_t        ppid;
        unsigned int state;
        long        exit_code;
        void        *cr3;

        long        tty;

        ListHead     list;

        WaitQueue    wait;

        pFile        fps[NR_OPENS];

    };

    unsigned char stack[TASK_SIZE];
} task_union;

task_union *alloc_task_union();


static inline task_union *get_current()
{
    task_union *tsk;
    asm("andl %%esp, %0;":"=r"(tsk):"0"(~(TASK_SIZE-1)));
    return tsk;
}

#define current get_current()

#define ROOT_TSK_PID    (0)

extern    ListHead    tsk_list;

#define add_tsk2list(tsk)    list_add_tail((&(tsk)->list), &tsk_list)
#define get_tsk_from_list(p)    list_entry((p), Task, list)
#define del_tsk_from_list(tsk)    list_del((&tsk->list))

#endif //_TASK_H

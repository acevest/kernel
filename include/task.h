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

#ifndef _TASK_H
#define _TASK_H

#define TASK_SIZE 4096

#define TI_preempt_cnt  0

#ifndef ASM
#include <page.h>
#include <list.h>
#include <types.h>
#include <processor.h>
#include <system.h>
#include <wait.h>
#include <fs.h>

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
        unsigned long   preempt_cnt;


        unsigned long    esp0;    /* kernel stack */

        /* for context switch */
        unsigned long    esp;
        unsigned long    eip;

        unsigned long   weight;

        pid_t        pid;
        pid_t        ppid;
        unsigned int state;
        long        exit_code;
        unsigned long cr3;

        long        tty;

        list_head_t list;

        wait_queue_t    wait;

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

#define ROOT_TSK_PID    (7)

#define TASK_INIT_WEIGHT 10

extern    ListHead    tsk_list;

#define add_tsk2list(tsk)    list_add_tail((&(tsk)->list), &tsk_list)
#define get_tsk_from_list(p)    list_entry((p), Task, list)
#define del_tsk_from_list(tsk)    list_del((&tsk->list))
#endif

#endif //_TASK_H

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

#define TI_preempt_cnt 0

#ifndef ASM
#include <fs.h>
#include <list.h>
#include <page.h>
#include <processor.h>
#include <system.h>
#include <types.h>

enum {
    TASK_UNUSED,
    TASK_RUNNING,
    TASK_WAIT,
    TASK_EXITING,
    TASK_END,
};

#define TASK_NAME_SIZE 32

typedef struct wait_queue_head {
    list_head_t task_list;
} wait_queue_head_t;

typedef union task_union {
    struct {
        unsigned long preempt_cnt;

        unsigned long esp0; /* kernel stack */

        /* for context switch */
        unsigned long esp;
        unsigned long eip;

        long weight;
        long priority;

        pid_t pid;
        pid_t ppid;
        unsigned int state;
        long exit_code;
        unsigned long cr3;

        long tty;

        char name[TASK_NAME_SIZE];

        list_head_t list;

        wait_queue_head_t wait;

        unsigned int cnt;  // debug only
    };

    unsigned char stack[TASK_SIZE];
} task_union;

task_union *alloc_task_union();

static inline task_union *get_current() {
    task_union *tsk;
    asm("andl %%esp, %0;" : "=r"(tsk) : "0"(~(TASK_SIZE - 1)));
    return tsk;
}

#define current get_current()

static inline pid_t sysc_getpid() { return current->pid; }

task_union *find_task(pid_t pid);

#define ROOT_TSK_PID (0)

#define TASK_INIT_WEIGHT 0

#define get_tsk_from_list(p) list_entry((p), Task, list)
#define del_tsk_from_list(tsk) list_del((&tsk->list))
#endif

#endif  //_TASK_H

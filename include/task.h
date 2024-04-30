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

#ifndef ASM
#include <fs.h>
#include <list.h>
#include <page.h>
#include <processor.h>
#include <system.h>
#include <types.h>
// #include <wait.h>

enum {
    TASK_UNUSED,
    TASK_RUNNING,
    TASK_READY,
    TASK_WAIT,
    TASK_INITING,
    TASK_EXITING,
    TASK_END,
};

#define TASK_NAME_SIZE 32

#define TASK_MAX_PRIORITY 99

#define TASK_MAGIC 0xAABBCCDD11223344

typedef union task_union {
    struct {
        uint32_t esp0; /* kernel stack */

        /* for context switch */
        uint32_t esp;
        uint32_t eip;

        uint32_t ticks;
        uint32_t turn;  // 时间片用完次数
        uint32_t priority;
        uint64_t jiffies;

        volatile int need_resched;

        pid_t pid;
        pid_t ppid;

        volatile unsigned int state;
        const char *reason;

        long exit_code;
        uint32_t cr3;

        long tty;

        char name[TASK_NAME_SIZE];

        list_head_t list;  // 所有进程串成一个链表

        list_head_t pend;  // 某些条件串成一个链表

        // list_head_t wait;

        uint32_t sched_cnt;       // 被调度换上CPU的次数
        uint32_t sched_keep_cnt;  // 时间片到了，但是没有被换出，又重新执行的次数

        uint64_t delay_jiffies;  // debug only

        uint64_t magic;  // 栈溢出标志
    };

    unsigned char stack[TASK_SIZE];
} task_t;

task_t *alloc_task_t();

static inline task_t *get_current() {
    task_t *tsk;
    asm("andl %%esp, %0;" : "=r"(tsk) : "0"(~(TASK_SIZE - 1)));
    return tsk;
}

#define current get_current()

static inline pid_t sysc_getpid() { return current->pid; }

task_t *find_task(pid_t pid);

#define ROOT_TSK_PID (0)

// #define TASK_INIT_WEIGHT 0

#define get_tsk_from_list(p) list_entry((p), Task, list)
#define del_tsk_from_list(tsk) list_del((&tsk->list))
#endif

#endif  //_TASK_H

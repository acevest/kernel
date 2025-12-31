/*
 *--------------------------------------------------------------------------
 *   File Name: exit.c
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Thu Mar  4 10:03:57 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */

#include <sched.h>
#include <system.h>
#include <wait.h>

int sysc_exit(int status) {
    unsigned long flags;
    irq_save(flags);
    current->state = TASK_EXITING;

    task_t* t = current;

    irq_restore(flags);

    return 0;
}

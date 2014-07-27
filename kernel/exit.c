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

int sysc_exit(int status)
{
    // simple implement.
    current->state = TASK_EXITING;

    task_union *t = current;
    
    schedule();

    return 0;
}

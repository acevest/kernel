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

    if(current == &RootTsk)
        panic("Root Task is Exiting...");

    /* 先简要实现 */
    current->state = TASK_EXITING;

    
    schedule();

    return 0;
}

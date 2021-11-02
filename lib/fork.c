/*
 *--------------------------------------------------------------------------
 *   File Name: fork.c
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sun Feb  7 13:30:24 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */

#include <stdio.h>
#include <syscall.h>
#include <types.h>
pid_t fork() {
#if 0
    pid_t pid;
    //asm("xchg %bx, %bx;");
    //syscall0(SYSC_FORK);
    //while(1);
    //asm("xchg %bx, %bx;");
    //asm("":"=a"(pid));
    //printf("pid:%x\n", pid);
    pid = syscall0(SYSC_FORK);
    return pid;
#endif
    return (pid_t)syscall0(SYSC_FORK);
}

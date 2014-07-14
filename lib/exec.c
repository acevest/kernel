/*
 *--------------------------------------------------------------------------
 *   File Name: exec.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Tue Feb 23 20:47:11 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <syscall.h>

int execv(const char *path, char *const argv[])
{
    return syscall2(SYSC_EXEC, path, argv);
}

int systest()
{
    return syscall0(SYSC_TEST);
}

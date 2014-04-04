/*
 *--------------------------------------------------------------------------
 *   File Name: exit.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Thu Mar  4 10:11:57 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */

#include <syscall.h>
int    exit(int status)
{
    syscall1(SYSC_EXIT, status);
}

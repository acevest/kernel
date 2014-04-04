/*
 *--------------------------------------------------------------------------
 *   File Name: open.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Tue Feb 23 01:15:29 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <syscall.h>
int open(const char *path, int flags, ...)
{
    // 不支持第三个参数
    return syscall3(SYSC_OPEN, path, flags, 0);
}

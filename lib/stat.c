/*
 *--------------------------------------------------------------------------
 *   File Name: stat.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Tue Feb 23 19:27:15 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <types.h>
#include <syscall.h>
#include <stat.h>

inline int _stat(int fd, struct stat *stat)
{
    return syscall2(SYSC_STAT, fd, stat);
}
int fstat(int fd, struct stat *buf)
{
    return _stat(fd, buf);
}

/*
 *--------------------------------------------------------------------------
 *   File Name: open.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sat Feb 20 18:53:47 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */

#include <fs.h>
#include <errno.h>
#include <types.h>
#include <sched.h>
#include <fcntl.h>
#include <assert.h>
#include <syscall.h>

int sysc_open(const char *path, int flags, mode_t mode)
{
    return 0;
}

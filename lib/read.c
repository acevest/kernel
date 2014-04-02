/*
 *--------------------------------------------------------------------------
 *   File Name:	read.c
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 			Tue Feb 23 18:54:48 2010
 * 
 * Description:	none
 * 
 *--------------------------------------------------------------------------
 */
#include<types.h>
#include<syscall.h>
ssize_t read(int fd, void *buf, size_t count)
{
	return (ssize_t) syscall3(SYSC_READ, fd, buf, count);
}

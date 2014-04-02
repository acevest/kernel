/*
 *--------------------------------------------------------------------------
 *   File Name:	write.c
 * 
 * Description:	none
 * 
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:	1.0
 * Create Date: Mon Mar  9 02:00:09 2009
 * Last Update: Mon Mar  9 02:00:09 2009
 * 
 *--------------------------------------------------------------------------
 */

#include<syscall.h>

int write(int fd, const char *buf, unsigned long size)
{
	//asm(""::"c"(size),"d"(buf),"b"(fd));
	//sysenter(0);
	//syscall3(0, fd, buf, size);
	//asm("nop;nop;nop;");

	syscall3(SYSC_WRITE, fd, buf, size);

	return size;
}

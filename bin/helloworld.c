/*
 *--------------------------------------------------------------------------
 *   File Name:	helloworld.c
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 			Tue Feb 23 22:44:40 2010
 * 
 * Description:	none
 * 
 *--------------------------------------------------------------------------
 */
#include<unistd.h>
#include<stdio.h>

int hw()
{
	printf("hello world\n");
	//syscall3(SYSC_WRITE, 0, "fuck", 5);
	//write(0, "hello world", sizeof("hello world"));
	//write(0, "hello world", 12);
	
	exit(0);

	return 0;
}

/*
 *--------------------------------------------------------------------------
 *   File Name: shell.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Wed Feb 24 17:47:22 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <stdio.h>
#include <types.h>
#include <string.h>

int systest();
int main()
{

    while(1)
    {
#if 0
    asm("movl $11, %eax;"   \
        "pushl $1f;"        \
        "pushl %ecx;"       \
        "pushl %edx;"       \
        "pushl %ebp;"       \
        "movl  %esp,%ebp;"  \
        "sysenter;"         \
        "1:");
#endif
        systest();
        asm("nop;nop;nop;");
    }

    return 0;
}

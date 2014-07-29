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
int sysdebug();
int pause(unsigned long tick);
int main()
{
    int pid = fork();
    printf("pid %u\n", pid);
    if(pid > 0)
    {
        while(1) {
            printf("parent. child pid %u\n", pid);
            systest();
            sysdebug(0x44444444);
        }
    }
    else
    {
        while(1) {
            printf("child\n");
            systest();
            sysdebug(0xAABBCCDD);
        }
    }

    return 0;
}

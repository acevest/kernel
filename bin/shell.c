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

    while(1)
    {
        printf("shell#");
        char buf[256];
        read(0, buf, 256);

        int pid = fork();
        if(pid > 0)
        {
            wait(pid);
        }
        else
        {
            execv(buf, 0);
        }
    }



    int pid = fork();
    printf("pid %u\n", pid);
    if(pid > 0)
    {
        printf("parent. child pid %u\n", pid);
        while(1) {
            systest();
            sysdebug(0x44444444);
        }
    }
    else
    {
        printf("child\n");
        while(1) {
            systest();
            sysdebug(0xAABBCCDD);
        }
    }

    return 0;
}

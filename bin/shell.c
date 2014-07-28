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
    if(pid > 0)
    {
        int n = 10000000;
        while(n--);
        printf("parent\n");
        while(1)
        {
        printf("parent\n");
            systest();
            //sysdebug(0x77777777);
            sysdebug(0x44444444);
        }
    }
    else
    {
        printf("child\n");
        while(1) {
        printf("child\n");
            systest();
            sysdebug(0xAABBCCDD);
            sysdebug(0x0A0B0C0D);
        }
    }

    if(pid > 0)
    {
        printf("prarent child pid %u\n", pid);
        wait(pid);
        printf(">prarent child pid %u\n", pid);
        while(1)
        {
            systest();
            sysdebug(0x112233);
        }
    }
    else if(pid == 0)
    {
        printf("child\n");
        //execv("/bin/hello", 0);
        printf(">child\n");
        while(1)
        {
            systest();
            sysdebug(0xAABBCCDD);
        }
    }
    else
    {

    }

    return 0;
}

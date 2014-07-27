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
int main()
{
    int pid = fork();

    if(pid > 0)
    {
        printf("prarent child pid %u\n", pid);
        while(1)
        {
            systest();
            sysdebug(0x112233);
        }
    }
    else if(pid == 0)
    {
        printf("child\n");
        execv("/bin/hello", 0);
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

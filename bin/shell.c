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

int main()
{

    while(1)
    {
        printf("shell#");
        char cmd[256];
        read(0, cmd, 256);

        int pid = fork();
        if(pid > 0)
        {
            wait(pid);
        }
        else
        {
            execv(cmd, 0);
            printf("failed to execute cmd: %s\n", cmd);
            exit(0);
        }
    }


    return 0;
}

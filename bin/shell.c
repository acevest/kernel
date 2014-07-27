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
    char x[10];
    x[0] = 'a';
    x[1] = 'b';
    x[2] = 'c';
    x[3] = '\n';
#if 1
    //while(1) write(0, x, 4);
    //while(1) systest();
    int pid = fork();

    if(pid > 0)
    {
        //write(0, "parent\n", 7);
        //write(0, x, 4);
        while(1)
        ;    systest();
    }
    else if(pid == 0)
#endif
    {
        //write(0, "child\n", 6);
        //write(0, x, 4);
        while(1)
        {
            systest();
        }
    }

    return 0;
}

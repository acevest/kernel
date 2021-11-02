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
#include <string.h>
#include <types.h>

void systest();
void sysdebug(unsigned int v);
int main() {
    while (1) {
        systest();
        sysdebug(0xAABBCCDD);

        printf("shell# ");
        char cmd[256];
        read(0, cmd, 256);

        int len = strlen(cmd);
        if (len > 0) cmd[len - 1] = 0;

        int pid = fork();
        if (pid > 0) {
            wait(pid);
        } else {
            execv(cmd, 0);
            printf("failed to execute cmd: %s\n", cmd);
            exit(0);
        }
    }

    return 0;
}

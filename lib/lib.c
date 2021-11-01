/*
 *--------------------------------------------------------------------------
 *   File Name: lib.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Wed Feb 17 18:58:13 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <syscall.h>
#include <system.h>

int isdigit(char c)
{
    return ('0' <= c && c <= '9');
}

int atoi(const char *s)
{
    int i = 0;
    while (isdigit(*s))
    {
        i *= 10;
        i += (*s++ - '0');
    }

    return i;
}

void reboot()
{
    syscall1(SYSC_REBOOT, 0);
}

void poweroff()
{
    syscall1(SYSC_REBOOT, 1);
}

int systest()
{
    return syscall0(SYSC_TEST);
}

int sysdebug(unsigned int v)
{
    return syscall1(SYSC_DEBUG, v);
}

int pause(unsigned long tick)
{
    return syscall1(SYSC_PAUSE, tick);
}

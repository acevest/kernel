/*
 *--------------------------------------------------------------------------
 *   File Name: system.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sun Jan 24 13:57:46 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */


#include <syscall.h>

int    sysc_reboot(int mode)
{

    void do_reboot();
    void do_poweroff();

    switch(mode)
    {
    case 0:
        do_reboot();
        break;
    case 1:
        do_poweroff();
        break;
    }

    return 0;
}



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


inline void panic(char *msg)
{
    printk("PANIC:\"%s\" file:%s function:%s line:%d\n",
        msg, __FILE__, __FUNCTION__, __LINE__);
    while(1);
}

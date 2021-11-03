/*
 *--------------------------------------------------------------------------
 *   File Name: cmdline.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Wed Feb 17 17:11:37 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <fs.h>
#include <printk.h>
#include <system.h>
#include <string.h>
#include <stdlib.h>

static void get_value(const char *name, char *value)
{
    const char *p;
    if(0 != (p = strstr(system.cmdline, name)) )
    {
        if(0 != (p = strstr(p, "=")))
        {
            p++;
            while(*p != ' ' && *p != 0)
                *value++ = *p++;
        }
    }

    *value = 0;
}


void parse_cmdline(const char *cmdline)
{
    char value[128];
    system.cmdline = cmdline;
    printk("cmdline: %s\n", system.cmdline);

    get_value("root", value);
    printk("root device %s\n", value);
    assert(value[0]=='h' && value[1]=='d' && value[2] == 'a');
    system.root_dev = MAKE_DEV(DEV_MAJOR_HDA, atoi(value+3));
    printk("root device %08x\n", system.root_dev);
}

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
#include <stdlib.h>
#include <string.h>
#include <system.h>

static void get_value(const char* name, char* value) {
    const char* p;
    if (0 != (p = strstr(system.cmdline, name))) {
        if (0 != (p = strstr(p, "="))) {
            p++;
            while (*p != ' ' && *p != 0) {
                *value++ = *p++;
            }
        }
    }

    *value = 0;
}

void parse_cmdline(const char* cmdline) {
    char value[128];
    system.cmdline = cmdline;
    printk("cmdline: %s\n", system.cmdline);

    get_value("root", value);

    assert(strlen(value) >= 4);

    assert(value[0] == 'h' && value[1] == 'd');
    assert(value[2] >= 'a' && value[2] <= 'd');

    int disk_drvid = value[2] - 'a';

    uint32_t partid = atoi(value + 3);
    assert(partid >= 1);

    system.root_dev = MAKE_DISK_DEV(disk_drvid, partid);
    printk("root device %s [0x%08x]\n", value, system.root_dev);

    get_value("delay", value);
    system.delay = atoi(value);
}

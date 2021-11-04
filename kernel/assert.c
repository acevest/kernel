/*
 *--------------------------------------------------------------------------
 *   File Name: assert.c
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sat Jan 23 15:25:29 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */

#include <printk.h>

void assert_fail(char *exp, char *file, unsigned int line, const char *func) {
    printk("%s:%d: %s: Assertion \'%s\' failed.\n", file, line, func, exp);

    while (1)
        ;
}

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

void assert_fail(char *exp, char *file, unsigned int line, char *func) {
    printk("%s:%d: %s: Assertion \'%s\' failed.\n", file, line, func, exp);

    while (1)
        ;
}

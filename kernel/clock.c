/*
 *--------------------------------------------------------------------------
 *   File Name: clock.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Tue Jan  5 09:51:54 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */

#include <printk.h>
#include <system.h>

static unsigned int jiffies = 0;

unsigned int sys_clock()
{
    return jiffies;
}

void clk_handler(unsigned int irq, pt_regs_t *regs, void *dev_id)
{
    jiffies++;

    //printd("^");
    printl(MPL_CLOCK, "clock irq: %d", jiffies);
}

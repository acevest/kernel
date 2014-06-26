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

void    clk_handler(unsigned int irq, pt_regs_t * regs, void *dev_id)
{
    jiffies++;

    //printk("^");
    //printd(0, "clock: %d", jiffies);
}

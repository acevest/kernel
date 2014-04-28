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

void    clk_handler(unsigned int irq, pPtRegs regs, void *dev_id)
{
    jiffies++;

    printk("^%d^ ", jiffies);
    //printk("%s ", dev_id);
}

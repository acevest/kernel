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
#include <sched.h>
#include <system.h>
#include <wait.h>

volatile uint32_t jiffies = 0;

unsigned int sys_clock() { return jiffies; }

void clk_handler(unsigned int irq, pt_regs_t *regs, void *dev_id) {
    if (jiffies % 100 == 0) {
        printl(MPL_CLOCK, "clock irq: %d", jiffies);
    }

    unsigned long iflags;
    irq_save(iflags);

    jiffies++;

    current->jiffies = jiffies;
    current->ticks--;
    assert(current->ticks <= TASK_MAX_PRIORITY);  // 防止ticks被减到0后再减溢出

    task_union *p = 0;
    list_head_t *t = 0;
    list_head_t *pos = 0;
    list_for_each_safe(pos, t, &all_tasks) {
        p = list_entry(pos, task_union, list);
        if (0 != p->delay_jiffies && jiffies > p->delay_jiffies && p->state == TASK_WAIT) {
            p->delay_jiffies = 0;
            p->state = TASK_READY;
        }
    }

    irq_restore(iflags);
}

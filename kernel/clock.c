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

static unsigned int jiffies = 0;

unsigned int sys_clock() { return jiffies; }

void clk_handler(unsigned int irq, pt_regs_t *regs, void *dev_id) {
    jiffies++;

    task_union *p = 0, *p1 = 0;
    list_head_t *pos = 0, *t = 0;

    unsigned long iflags;
    irq_save(iflags);
    int cnt = 0;

    list_for_each_entry_safe(p, p1, &pend_tasks, pend) {
        // printk("cnt %d %d\n", cnt++, p->delay_cnt);

        p->delay_cnt -= p->delay_cnt == 0 ? 0 : 1;

        if (0 == p->delay_cnt) {
            p->state = TASK_RUNNING;
            list_del(&p->pend);
        }
    }

    irq_restore(iflags);

    // printd("^");
    // printl(MPL_CLOCK, "clock irq: %d", jiffies);
}

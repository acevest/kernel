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

volatile uint32_t jiffies = 0;  // TODO uint64

unsigned int sys_clock() { return jiffies; }

void debug_print_all_tasks();

void dump_irq_nr_stack();
void clk_handler(unsigned int irq, pt_regs_t *regs, void *dev_id) {
    if (jiffies % 100 == 0) {
        printl(MPL_CLOCK, "clock irq: %d", jiffies);
    }

    unsigned long iflags;
    irq_save(iflags);

    jiffies++;

    current->jiffies = jiffies;
    // 若clk_handler嵌套在其它中断函数中执行
    // 那么这个变量减到0时，再退出中clk_handler并不会引起调度
    //

    current->ticks--;
    if (current->ticks > TASK_MAX_PRIORITY) {
        printl(MPL_X, "current %08x ticks %u", current, current->ticks);
        printk("DIE: ");
        dump_irq_nr_stack();
    }
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

    debug_print_all_tasks();
    debug_print_all_tasks();
    debug_print_all_tasks();
    debug_print_all_tasks();

    irq_restore(iflags);
}

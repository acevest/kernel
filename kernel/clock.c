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

#if 0
    unsigned long iflags;
    irq_save(iflags);
#endif
    jiffies++;

    current->jiffies = jiffies;

    // 中断目前虽然不能嵌套，但依然可以打断前一个中断的下半部分处理
    // 若前一个时钟中断将这个值减到0
    // 同时其下半部分处理时间过长，直到这个时钟中断还没处理完
    // 那么这个时钟中断是完全可以打断它，且在这里把这个ticks从0减到负数
    // 而这个是uint32_t型，因此会溢出成0xFFFFFFFF
    volatile uint32_t ticks;

    ticks = current->ticks;
    if (current->ticks > 0) {
        current->ticks--;
    }
    ticks = current->ticks;

#if 0
    if (current->ticks > TASK_MAX_PRIORITY) {
        printl(MPL_X, "current %08x ticks %u", current, current->ticks);
        printk("DIE: ");
        dump_irq_nr_stack();
    }
#endif

    assert(current->ticks <= TASK_MAX_PRIORITY);  // 防止ticks被减到0后再减溢出

#if 0
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
#endif

#if 0
    debug_print_all_tasks();
    debug_print_all_tasks();
    debug_print_all_tasks();
    debug_print_all_tasks();
#endif

#if 0
    irq_restore(iflags);
#endif
}

// 开中断执行这个函数
const char *task_state(unsigned int state);
void clk_bh_handler() {
    unsigned long iflags;
    irq_save(iflags);
    task_union *p = 0;
    list_head_t *t = 0;
    list_head_t *pos = 0;
    list_for_each_safe(pos, t, &delay_tasks) {
        p = list_entry(pos, task_union, pend);
        // printk("%s state: %s\n", p->name, task_state(p->state));
        assert(p->state == TASK_WAIT);
        assert(p->delay_jiffies != 0);
        if (jiffies > p->delay_jiffies) {
            list_del(&p->pend);
            p->delay_jiffies = 0;
            p->state = TASK_READY;
        }
    }

    // 此处调用这个还是有问题的
    debug_print_all_tasks();
    irq_restore(iflags);
}
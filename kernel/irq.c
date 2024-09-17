/*
 *--------------------------------------------------------------------------
 *   File Name: irq.c
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Thu Jul 16 18:06:22 2009
 * Last Update: Thu Jul 16 18:06:22 2009
 *
 *--------------------------------------------------------------------------
 */

#include <assert.h>
#include <atomic.h>
#include <errno.h>
#include <irq.h>
#include <system.h>
#include <task.h>

irq_desc_t irq_desc[NR_IRQS];
irq_bh_action_t *irq_bh_actions = NULL;
irq_bh_action_t *irq_bh_actions_end = NULL;

#if 0
unsigned int irq_nr_stack[64] = {1, 2, 3, 4};
uint32_t irq_nr_jiffies_stack[64] = {
    0,
};
int irq_nr_stack_pos = 0;

extern uint32_t jiffies;
void push_irq_nr_stack(unsigned int irq) {
    irq_nr_stack[irq_nr_stack_pos] = irq;
    irq_nr_jiffies_stack[irq_nr_stack_pos] = jiffies;
    irq_nr_stack_pos++;
}

unsigned int pop_irq_nr_stack() {
    irq_nr_stack_pos--;
    return irq_nr_stack[irq_nr_stack_pos];
}


int vsprintf(char *buf, const char *fmt, char *args);
void dump_irq_nr_stack() {
    if (irq_nr_stack_pos == 0) {
        printl(MPL_TEST, "irq nr stack empty");
        return;
    }

    printl(MPL_DEBUG, "irq nr stack pos %u", irq_nr_stack_pos);

    char buf[128];

    memset(buf, 0, 128);
    strcpy(buf, "irq nr stack: ");
    for (int i = 0; i < irq_nr_stack_pos; i++) {
        char dbuf[64];
        vsprintf(dbuf, "%02d:", irq_nr_stack + i);  // 这里vsprintf有坑，坑点在第三个参数不是标准参数
        strcat(buf, dbuf);
        vsprintf(dbuf, "%d ", irq_nr_jiffies_stack + i);
        strcat(buf, dbuf);
    }

    printl(MPL_TEST, "                                                                               ");

    printl(MPL_TEST, buf);
    strcat(buf, "\n");
    printk(buf);
}
#endif

void irq_bh_handler();
void schedule();

volatile int reenter_count = 0;

volatile uint32_t clk_irq_cnt = 0;

__attribute__((regparm(1))) void irq_handler(pt_regs_t *regs) {
    unsigned int irq = regs->irq;
    if (irq >= NR_IRQS) {
        panic("invalid irq %d\n", irq);
    }

    irq_desc_t *p = irq_desc + irq;
    irq_action_t *action = p->action;

    // 在qemu启动后如果gdb有加断点，就很会一直触发中断重入
    reenter++;
    reenter_count += reenter == 0 ? 0 : 1;
    assert(irq_disabled());
    assert(reenter >= 0);
    assert(reenter <= 1);

    // TODO 判断打断的是否是内核态代码

    // 屏蔽当前中断
    p->chip->disable(irq);

    // 发送EOI
    p->chip->ack(irq);

#if 0
    if (0x00 == irq) {
        if ((clk_irq_cnt++ & 0xFU) != 0) {
            reenter--;
            p->chip->enable(irq);
            return;
        }
    }
#endif

    assert(current->magic == TASK_MAGIC);

#if 1
    unsigned long esp;
    asm("movl %%esp, %%eax" : "=a"(esp));
    printl(MPL_CURRENT, "current %08x %-6s cr3 %08x reenter %d:%u esp %08x ticks %u", current, current->name,
           current->cr3, reenter, reenter_count, esp, current->ticks);
#endif

    while (action && action->handler) {
        action->handler(irq, regs, action->dev_id);
        action = action->next;
    }

    // 解除屏蔽当前中断
    p->chip->enable(irq);

    // 代表当前中断程序打断了前一个中断程序的“开中断处理的底半部分逻辑”
    // 即前一个中断处理尚未完全完成
    assert(irq_disabled());
    if (reenter != 0) {
        reenter--;
        return;
    }
    // --以上逻辑CPU处于中断禁止状态--------------------------

    // 此处执行中断函数的下半部分逻辑，开中断执行
    {
        enable_irq();

        // 这里面不能执行任务切换操作，比如信号量相关操作
        irq_bh_handler();

        disable_irq();
    }

    // --以下逻辑CPU处于中断禁止状态--------------------------
    assert(irq_disabled());
    assert(reenter == 0);
    reenter--;

    // 考察如果不需要调度程序，直接退出
    if (current->need_resched == 0) {
        return;
    }

    // if (irq != 0) {
    //     return;
    // }

    enable_irq();

    // 如果需要调度程序
    schedule();
}

extern uint32_t jiffies;

volatile bool enable_clock_irq_delay = false;

void irq_bh_handler() {
    uint32_t end = jiffies + 1;

// ENABLE_CLOCK_IRQ_WAIT是用来调试的
// 是为了让时钟减缓进程的时间片更慢一点，以便于调试
// 采用这种方式，而不是经过一定时钟数再减一次进程时间片的方法
// 是因为这种方法只能让时间片减得慢，但会拉长进程的实际运行时间，效果不真实
// 这种方法不会改变进程的运行时间
#if ENABLE_CLOCK_IRQ_WAIT
    uint32_t debug_end = jiffies + 20;
    while (jiffies < debug_end)
#endif
    {
        while (true) {
            irq_bh_action_t *action = NULL;

#if 1
            disable_irq();
            action = irq_bh_actions;
            if (action == NULL) {
                enable_irq();
                break;
            }

            irq_bh_actions = action->next;
            if (irq_bh_actions == NULL) {
                irq_bh_actions_end = NULL;
            }
            enable_irq();

            action->handler(action->arg);
            kfree(action);
#else
            disable_irq();

            action = irq_bh_actions;
            irq_bh_actions = NULL;
            irq_bh_actions_end = NULL;

            enable_irq();

            if (action == NULL) {
                break;
            }

            while (action != NULL) {
                action->handler(action->arg);
                irq_bh_action_t *p = action;
                action = action->next;
                kfree(p);
            }
#endif

            if (jiffies >= end) {
                break;
            }

            // 这里可能存在有部分没处理完
        }

        void debug_print_all_tasks();
#if ENABLE_CLOCK_IRQ_WAIT
        debug_print_all_tasks();
        if (irq_bh_actions == NULL) {
            asm("hlt;");
        }
#else
        if (jiffies < end) {
            debug_print_all_tasks();
        }
#endif
    }

#if ENABLE_CLOCK_IRQ_WAIT
    enable_clock_irq_delay = false;
#endif

    // 这之后也可能存在再次被中断加入下半部处理请求
    // 但这些都不会丢失
    // 而是会延迟到这次中断返回后下一次的中断的下半部分处理逻辑
}

int request_irq(unsigned int irq, void (*handler)(unsigned int, pt_regs_t *, void *), const char *devname,
                void *dev_id) {
    irq_action_t *p;

    if (irq >= NR_IRQS) {
        return -EINVAL;
    }
    if (handler == NULL) {
        return -EINVAL;
    }

    // 检查是否已经注册过处理函数了
    p = irq_desc[irq].action;
    while (p != NULL) {
        if (p->handler == handler) {
            return 0;
        }
        p = p->next;
    }

    p = (irq_action_t *)kmalloc(sizeof(irq_action_t), 0);
    if (p == NULL) {
        return -ENOMEM;
    }

    p->dev_name = devname;
    p->dev_id = dev_id;
    p->handler = handler;
    p->next = NULL;
    if (irq_desc[irq].action != NULL) {
        p->next = irq_desc[irq].action;
        // printk("p->next:%x\n", p->next);
    }
    irq_desc[irq].action = p;
    // printk("irq: %d action:%x\n", irq, p);
    return 0;
}

void add_irq_bh_handler(void (*handler)(), void *arg) {
    // 只能在中断处理函数中调用
    assert(irq_disabled());
    assert(reenter >= 0);

    // 本函数不用考虑临界问题

    irq_bh_action_t *p;
    p = (irq_bh_action_t *)kmalloc(sizeof(irq_bh_action_t), 0);
    assert(p != NULL);

    p->handler = handler;
    p->arg = arg;

    if (irq_bh_actions_end == NULL) {
        assert(irq_bh_actions == NULL);
        irq_bh_actions = p;
        irq_bh_actions_end = p;
    } else {
        assert(irq_bh_actions != NULL);
        irq_bh_actions_end->next = p;
        irq_bh_actions_end = p;
    }
    p->next = NULL;
}

int open_irq(unsigned int irq) { return irq_desc[irq].chip->enable(irq); }

int close_irq(unsigned int irq) { return irq_desc[irq].chip->disable(irq); }

bool irq_enabled() {
    uint32_t flags;
    __asm__ __volatile__("pushfl; popl %0;" : "=a"(flags));

    if ((flags & (1 << 9)) != 0) {
        return true;
    }

    return false;
}

bool irq_disabled() { return !irq_enabled(); }

int enable_no_irq_chip(unsigned int irq) { return 0; }
int disable_no_irq_chip(unsigned int irq) { return 0; }

irq_chip_t no_irq_chip = {.name = "none", .enable = enable_no_irq_chip, .disable = disable_no_irq_chip};
irq_desc_t no_irq_desc = {.chip = &no_irq_chip, .action = NULL, .status = 0, .depth = 0};

// 单CPU
static volatile uint32_t __critical_zone_eflags;
void enter_critical_zone() { irq_save(__critical_zone_eflags); }
void exit_critical_zone() { irq_restore(__critical_zone_eflags); }

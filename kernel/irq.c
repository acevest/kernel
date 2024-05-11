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

__attribute__((regparm(1))) void irq_handler(pt_regs_t *regs) {
    unsigned int irq = regs->irq;
    if (irq >= NR_IRQS) {
        panic("invalid irq %d\n", irq);
    }

    irq_desc_t *p = irq_desc + irq;
    irq_action_t *action = p->action;

    assert(irq_disabled());
    reenter++;

    // 屏蔽当前中断
    p->chip->disable(irq);

    // 发送EOI
    p->chip->ack(irq);

    assert(current->magic == TASK_MAGIC);

#if 1
    unsigned long esp;
    asm("movl %%esp, %%eax" : "=a"(esp));
    printl(MPL_CURRENT, "current %08x %-6s cr3 %08x reenter %d esp %08x ticks %u", current, current->name, current->cr3,
           reenter, esp, current->ticks);
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

    // 如果需要调度程序
    schedule();
}

extern uint32_t jiffies;
void irq_bh_handler() {
    uint32_t end = jiffies + 2;
    while (true) {
        irq_bh_action_t *action = NULL;

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

        if (jiffies >= end) {
            break;
        }

        // 这里可能存在有部分没处理完
    }

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

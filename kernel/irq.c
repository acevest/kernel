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

int enable_no_irq_chip(unsigned int irq) { return 0; }
int disable_no_irq_chip(unsigned int irq) { return 0; }

irq_chip_t no_irq_chip = {.name = "none", .enable = enable_no_irq_chip, .disable = disable_no_irq_chip};

irq_desc_t no_irq_desc = {.chip = &no_irq_chip, .action = NULL, .status = 0, .depth = 0};

__attribute__((regparm(1))) void irq_handler(pt_regs_t *regs) {
    unsigned int irq = regs->irq;
    if (irq >= NR_IRQS) {
        printk("invalid irq %d\n", irq);
        return;
    }

    irq_desc_t *p = irq_desc + irq;
    irq_action_t *action = p->action;

    // 屏蔽当前中断
    p->chip->disable(irq);

    // 发送EOI
    p->chip->ack(irq);

    if (irq_reenter == 0) {
        // 可以切换到中断栈
    }

    irq_reenter++;

    // 开中断执行中断处理函数
    enable_irq();

    unsigned long esp;
    asm("movl %%esp, %%eax" : "=a"(esp));
    printl(MPL_CURRENT, "current %08x cr3 %08x reenter %d esp %08x", current, current->cr3, irq_reenter, esp);

    while (action && action->handler) {
        action->handler(irq, regs, action->dev_id);
        action = action->next;
    }

    // 关全局中断
    disable_irq();

    irq_reenter--;

    // 解除屏蔽当前中断
    p->chip->enable(irq);
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
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

#include <irq.h>
#include <errno.h>
#include <assert.h>
#include <task.h>
#include <atomic.h>

irq_desc_t irq_desc[NR_IRQS];

int enable_no_irq_chip(unsigned int irq){return 0;}
int disable_no_irq_chip(unsigned int irq){return 0;}

irq_chip_t no_irq_chip =
{
    .name   = "none",
    .enable = enable_no_irq_chip,
    .disable= disable_no_irq_chip
};

irq_desc_t no_irq_desc =
{
    .chip   = &no_irq_chip,
    .action = NULL,
    .status = 0,
    .depth  = 0
};

static int preempt = 0;

__attribute__ ((regparm(1))) void irq_handler(pt_regs_t *regs)
{
    unsigned int irq = regs->irq;

    if(irq >= NR_IRQS)
    {
        printk("invalid irq %d\n", irq);
        return ;
    }

    irq_desc_t *p = irq_desc + irq;

    irq_action_t *action = p->action;

    //atomic_inc(&(current->preempt_cnt));
    atomic_inc(&preempt);

    unsigned long esp;
    printd(8, "preempt : %d", preempt);
    //asm("movl %%esp, %%eax":"=a"(esp));
    //printd(8, "preempt_cnt:%d current %08x esp %08x", current->preempt_cnt, current, esp);
    //printk("preempt_cnt:%d current %08x esp %08x\n", current->preempt_cnt, current, esp);

#if 0
    esp >>= 16;
    if(esp != 0xC013 && esp != 0xC7FF)
    {
        asm("cli");
        printk("FUCK\n");
        while(1);
    }
#endif

    p->chip->ack(irq);
    sti();
    while(action && action->handler)
    {
        action->handler(irq, regs, action->dev_id);
        action = action->next;
    }
    cli();
    p->chip->enable(irq);

    atomic_dec(&preempt);
    //atomic_dec(&(current->preempt_cnt));
}


int request_irq(unsigned int irq,
                void (*handler)(unsigned int, pt_regs_t *, void *),
                const char *devname,
                void *dev_id)
{
    irq_action_t *    p;

    if(irq >= NR_IRQS)
        return -EINVAL;
    if(handler == NULL)
        return -EINVAL;

    // 检查是否已经注册过处理函数了
    p = irq_desc[irq].action;
    while(p != NULL)
    {
        if(p->handler == handler)
            return 0;
        p = p->next;
    }

    p = (irq_action_t *) kmalloc(sizeof(irq_action_t), 0);
    if(p == NULL)
        return -ENOMEM;


    p->dev_name        = devname;
    p->dev_id        = dev_id;
    p->handler        = handler;
    p->next            = NULL;
    if(irq_desc[irq].action != NULL)
    {
        p->next    = irq_desc[irq].action;
        //printk("p->next:%x\n", p->next);
    }
    irq_desc[irq].action = p;
    //printk("irq: %d action:%x\n", irq, p);
    return 0;
}



int open_irq(unsigned int irq)
{
    return irq_desc[irq].chip->enable(irq);
}

int close_irq(unsigned int irq)
{
    return irq_desc[irq].chip->disable(irq);
}

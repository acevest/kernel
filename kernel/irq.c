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

__attribute__ ((regparm(1))) void irq_handler(pt_regs_t *regs)
{

    unsigned int irq = regs->irq;
    irq_desc_t *p = irq_desc + irq;
    irq_action_t *action = p->action;

    p->chip->ack(irq);
    sti();
    while(action)
    {
        //action->handler(regs, irq);
        action->handler(irq, regs, action->dev_id);
        action = action->next;
    }

    p->chip->enable(irq);
}


int    request_irq(    unsigned int irq,
            void    (*handler)(unsigned int, pt_regs_t *, void *),
            const char *devname,
            void    *dev_id)
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

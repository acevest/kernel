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

IrqDesc    irq_desc[NR_IRQS];

int    enable_no_irq_chip(unsigned int irq){return 0;}
int    disable_no_irq_chip(unsigned int irq){return 0;}
IrqChip    no_irq_chip =
{
    .name        = "none",
    .enable        = enable_no_irq_chip,
    .disable    = disable_no_irq_chip
};
IrqDesc    no_irq_desc =
{
    .chip    = &no_irq_chip,
    .action    = NULL,
    .status    = 0,
    .depth    = 0
};
__attribute__ ((regparm(1))) void    irq_handler(pPtRegs regs)
{

    unsigned int irq = regs->irq;
    pIrqDesc    p = irq_desc + irq;
    pIrqAction    action = p->action;
    p->chip->ack(irq);
    while(action)
    {
        //action->handler(regs, irq);
        action->handler(irq, regs, action->dev_id);
        action = action->next;
    }
    p->chip->enable(irq);
}


/*
int    request_irq(    unsigned int irq,
            void (*handler)(pPtRegs, unsigned int),
            const char *devname,
            void    *dev_id)
*/
int    request_irq(    unsigned int irq,
            void    (*handler)(unsigned int, pPtRegs, void *),
            const char *devname,
            void    *dev_id)
{
    pIrqAction    p;

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

    p = kmalloc_old(sizeof(IrqAction));
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

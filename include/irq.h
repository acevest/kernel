/*
 *--------------------------------------------------------------------------
 *   File Name: irq.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Fri Nov 28 16:38:25 2008
 * Last Update: Fri Nov 28 16:38:25 2008
 * 
 *--------------------------------------------------------------------------
 */

#ifndef    _IRQ_H
#define _IRQ_H

#include "system.h"

#define NR_IRQS        224
#define FIRST_IRQ_VECT    0x20

typedef    struct
{
    const char *name;
    int     (*enable)(unsigned int irq);
    int     (*disable)(unsigned int irq);
    void    (*ack)(unsigned int irq);
} IrqChip, *pIrqChip;

typedef struct irqaction
{
    //void (*handler)(pPtRegs regs, unsigned int irq);
    void    (*handler)(unsigned int irq, pPtRegs regs, void *dev_id);
    const char *dev_name;
    void *dev_id;
    struct irqaction *next;    
} IrqAction, *pIrqAction;

typedef    struct
{
    pIrqChip    chip;
    pIrqAction    action;
    unsigned int    status;
    unsigned int    depth;
} IrqDesc, *pIrqDesc;

extern    IrqChip    i8259_chip;
extern    IrqDesc    irq_desc[];
extern    IrqDesc    no_irq_desc;
int    request_irq(unsigned int irq,
    //void (*handler)(pPtRegs, unsigned int),
    void    (*handler)(unsigned int, pPtRegs, void *),
    const char *devname,
    void    *dev_id);

static inline int enable_irq(unsigned int irq)
{
    return irq_desc[irq].chip->enable(irq);
}
static inline int disable_irq(unsigned int irq)
{
    return irq_desc[irq].chip->disable(irq);
}
#endif //_IRQ_H

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

#ifndef _IRQ_H
#define _IRQ_H

#include "system.h"

#define FIRST_IRQ_VECT    0x20
#define NR_IRQS           (0xFF-FIRST_IRQ_VECT)

typedef struct irq_chip
{
    const char *name;
    int     (*enable)(unsigned int irq);
    int     (*disable)(unsigned int irq);
    void    (*ack)(unsigned int irq);
} irq_chip_t;

typedef struct irqaction
{
    //void (*handler)(pPtRegs regs, unsigned int irq);
    void    (*handler)(unsigned int irq, pPtRegs regs, void *dev_id);
    const char *dev_name;
    void *dev_id;
    struct irqaction *next;    
} irq_action_t;

typedef struct irq_desc
{
    irq_chip_t *    chip;
    irq_action_t *    action;
    unsigned int    status;
    unsigned int    depth;
} irq_desc_t;

extern    irq_chip_t    i8259_chip;
extern    irq_desc_t    irq_desc[];
extern    irq_desc_t    no_irq_desc;
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

/*
 *--------------------------------------------------------------------------
 *   File Name: i8259.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Sun Nov  9 11:37:09 2008
 *     Version: 1.1
 * Last Update: Tue Feb 10 20:28:47 2009
 * 
 *--------------------------------------------------------------------------
 */

#ifndef    _I8259_H
#define _I8259_H

#include "irq.h"
#include "io.h"

#define PIC_MASTER_CMD    0x20
#define PIC_MASTER_IMR    0x21
#define PIC_SLAVER_CMD    0xA0
#define PIC_SLAVER_IMR    0xA1

#define PIC_MASTER_ISR    PIC_MASTER_CMD
#define PIC_SLAVER_ISR    PIC_SLAVER_CMD

#define PIC_CASCADE_IR    0x2    //The IR2 on Master Connect to Slaver.

extern void init_i8259();
extern void mask_i8259();

static inline int enable_i8259_irq(unsigned int irq)
{
    unsigned char mask = ~(1 << irq);
    if(irq & 8)
    {
        mask &= inb(PIC_SLAVER_IMR);
        outb( mask, PIC_SLAVER_IMR);
    }
    else
    {
        mask &= inb(PIC_MASTER_IMR);
        outb_p( mask, PIC_MASTER_IMR);
    }
}
static inline int disable_i8259_irq(unsigned int irq)
{
    unsigned char mask = 1 << irq;
    if(irq & 8)
    {
        mask |= inb(PIC_SLAVER_IMR);
        outb( mask, PIC_SLAVER_IMR);
    }
    else
    {
        mask |= inb(PIC_MASTER_IMR);
        outb( mask, PIC_MASTER_IMR);
    }
}

static inline void mask_ack_i8259_irq(unsigned int irq)
{
    unsigned int mask = 1 << irq;

    if(irq & 8)
    {
        mask |= inb(PIC_SLAVER_IMR);
        // Mask
        outb(mask, PIC_SLAVER_IMR);
        // Specific EOI to slave
        outb(0x60 + (irq & 0x07), PIC_SLAVER_CMD);
        // Specific EOI to master
        outb(0x60 + (PIC_CASCADE_IR & 0x07), PIC_MASTER_CMD);
    }
    else
    {
        mask |= inb(PIC_MASTER_IMR);
        // Mask
        outb(mask, PIC_MASTER_IMR);
        // Specific EOI to master
        outb(0x60 + irq, PIC_MASTER_CMD);
    }

/*
    // None Specific EOI
    outb(0x20, PIC_MASTER_CMDA);
    outb(0x20, PIC_SLAVER_CMDA);
*/
}


static inline void do_i8259_IRQ(pPtRegs regs, unsigned int irq)
{
}



#endif //_I8259_H

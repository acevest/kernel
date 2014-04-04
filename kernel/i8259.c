/*
 *--------------------------------------------------------------------------
 *   File Name: i8259.c
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Sun Nov  9 11:35:22 2008
 * Last Update: Tue Feb 10 22:37:15 2009
 * 
 *--------------------------------------------------------------------------
 */


#include "io.h"
#include "i8259.h"
#include "irq.h"

IrqChip    i8259_chip =
{
    .name        = "XT-PIC",
    .enable        = enable_i8259_irq,
    .disable    = disable_i8259_irq,
    .ack        = mask_ack_i8259_irq,
};
#if 0
void enable_i8259_irq(unsigned int irq)
{
    unsigned int mask = ~(1 << irq);
    cached_irq_mask &= mask;
    if(irq & 8)
        outb_p(cached_slaver_mask, PIC_SLAVER_IMR);
    else
        outb_p(cached_master_mask, PIC_MASTER_IMR);
}

void disable_i8259_irq(unsigned int irq)
{
    unsigned int mask = 1 << irq;
    cached_irq_mask |= mask;
    if(irq & 8)
        outb_p(cached_slaver_mask, PIC_SLAVER_IMR);
    else
        outb_p(cached_master_mask, PIC_MASTER_IMR);
}
#endif
void mask_i8259()
{
    //mask all of 8259
    outb_p(0xFF, PIC_MASTER_IMR);
    outb_p(0xFF, PIC_SLAVER_IMR);
}
void init_i8259()
{
#if 0
    outb_p(0x11,0x20);
    outb_p(0x11,0xA0);

    outb_p(0x20,0x21);
    outb_p(0x28,0xA1);

    outb_p(0x04,0x21);
    outb_p(0x02,0xA1);

    outb_p(0x01,0x21);
    outb_p(0x01,0xA1);

    outb_p(0xFF,0x21);
    outb_p(0xFF,0xA1);
#else
    //Master...
    outb_p(0x11, PIC_MASTER_CMD); // ICW1
    outb_p(0x20, PIC_MASTER_IMR); // ICW2: IR0-7 mapped to 0x20-0x27
    outb_p(1U<<PIC_CASCADE_IR, PIC_MASTER_IMR); //IR2 Connect Slaver.
    outb_p(0x01, PIC_MASTER_IMR);    // Normal EOI
    //Auto EOI:outb_p(0x03, PIC_MASTER_CMDB);
    
    //Slaver...
    outb_p(0x11, PIC_SLAVER_CMD);
    outb_p(0x28, PIC_SLAVER_IMR); // IR0-7 mapped to 0x28-0x2F
    outb_p(PIC_CASCADE_IR, PIC_SLAVER_IMR);
    outb_p(0x01, PIC_SLAVER_IMR);
    //Auto EOI:outb_p(0x01, PIC_SLAVER_CMDB);
    mask_i8259();
#endif
}

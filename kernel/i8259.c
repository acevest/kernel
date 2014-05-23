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

void mask_i8259()
{
    //mask all of 8259
    outb_p(0xFF, PIC_MASTER_IMR);
    outb_p(0xFF, PIC_SLAVE_IMR);
    //outb_p(0x0, PIC_MASTER_IMR);
    //outb_p(0x0, PIC_SLAVE_IMR);
}

void init_i8259()
{
    //Master...
    outb_p(0x11, PIC_MASTER_CMD); // ICW1
    outb_p(0x20, PIC_MASTER_IMR); // ICW2: IR0-7 mapped to 0x20-0x27
    outb_p((1U<<PIC_CASCADE_IR), PIC_MASTER_IMR); //IR2 Connect Slave.
#if PIC_AEOI
    outb_p(0x03, PIC_MASTER_IMR);
#else
    outb_p(0x01, PIC_MASTER_IMR);    // Normal EOI
#endif
    
    //Slave...
    outb_p(0x11, PIC_SLAVE_CMD);
    outb_p(0x28, PIC_SLAVE_IMR); // IR0-7 mapped to 0x28-0x2F
    outb_p(PIC_CASCADE_IR, PIC_SLAVE_IMR);
#if PIC_AEOI
    outb_p(0x03, PIC_SLAVE_IMR);
#else
    outb_p(0x01, PIC_SLAVE_IMR);
#endif
    mask_i8259();
}

int enable_i8259_irq(unsigned int irq)
{
    unsigned char mask = 0;
    if(irq & 8)
    {
        mask = ~(1 << (irq-8));
        mask &= inb(PIC_SLAVE_IMR);
        outb( mask, PIC_SLAVE_IMR);
    }
    else
    {
        mask = ~(1 << irq);
        mask &= inb(PIC_MASTER_IMR);
        outb_p( mask, PIC_MASTER_IMR);
    }
}

int disable_i8259_irq(unsigned int irq)
{
    unsigned char mask = 0;
    if(irq & 8)
    {
        mask |= (1 << (irq-8));
        mask |= inb(PIC_SLAVE_IMR);
        outb( mask, PIC_SLAVE_IMR);
    }
    else
    {
        mask |= (1 << irq);
        mask |= inb(PIC_MASTER_IMR);
        outb( mask, PIC_MASTER_IMR);
    }
}

void mask_ack_i8259_irq(unsigned int irq)
{
    unsigned char mask = 0;

    if(irq & 8) // Slave
    {
        mask |= (1 << (irq-8));
        mask |= inb(PIC_SLAVE_IMR);
        // Mask
        outb(mask, PIC_SLAVE_IMR);
#if PIC_AEOI
        // ...
#else
        // Specific EOI to slave
        outb(0x60 + (irq & 0x07), PIC_SLAVE_CMD);
        // Specific EOI to master
        outb(0x60 + (PIC_CASCADE_IR & 0x07), PIC_MASTER_CMD);
#endif
    }
    else        // Master
    {
        mask |= (1 << irq);
        mask |= inb(PIC_MASTER_IMR);
        // Mask
        outb(mask, PIC_MASTER_IMR);
#if PIC_AEOI
        // ...
#else
        // Specific EOI to master
        outb(0x60 + irq, PIC_MASTER_CMD);
#endif
    }
}

irq_chip_t    i8259_chip =
{
    .name       = "XT-PIC",
    .enable     = enable_i8259_irq,
    .disable    = disable_i8259_irq,
    .ack        = mask_ack_i8259_irq,
};

void do_i8259_IRQ(pt_regs_t * regs, unsigned int irq)
{


}

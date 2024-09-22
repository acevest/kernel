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

#define FIRST_IRQ_VECT 0x20
#define NR_IRQS (0xFF - FIRST_IRQ_VECT)

typedef struct irq_chip {
    const char *name;
    int (*enable)(unsigned int irq);
    int (*disable)(unsigned int irq);
    void (*ack)(unsigned int irq);
} irq_chip_t;

typedef struct irqaction {
    // void (*handler)(pt_regs_t * regs, unsigned int irq);
    void (*handler)(unsigned int irq, pt_regs_t *regs, void *dev_id);
    const char *dev_name;
    void *dev_id;
    struct irqaction *next;
} irq_action_t;

typedef struct irq_desc {
    irq_chip_t *chip;
    irq_action_t *action;
    unsigned int status;
    unsigned int depth;
} irq_desc_t;

typedef struct irq_bh_action {
    void (*handler)();
    void *arg;
    struct irq_bh_action *next;
} irq_bh_action_t;

extern irq_chip_t i8259_chip;
extern irq_desc_t irq_desc[];
extern irq_desc_t no_irq_desc;
int request_irq(unsigned int irq,
                // void (*handler)(pt_regs_t *, unsigned int),
                void (*handler)(unsigned int, pt_regs_t *, void *), const char *devname, void *dev_id);

void add_irq_bh_handler(void (*handler)(), void *arg);

int open_irq(unsigned int irq);
int close_irq(unsigned int irq);

#define enable_irq() asm("sti")
#define disable_irq() asm("cli")

#define irq_save(x) __asm__ __volatile__("pushfl; popl %0; cli" : "=g"(x)::"memory")

bool irq_enabled();
bool irq_disabled();

#define irq_restore(x)                                                     \
    do {                                                                   \
        typecheck(unsigned long, x);                                       \
        __asm__ __volatile__("pushl %0; popfl" ::"g"(x) : "memory", "cc"); \
    } while (0)

void enter_critical_zone();
void exit_critical_zone();

#define ENTER_CRITICAL_ZONE(x) \
        volatile uint32_t __critical_zone_eflags_##x = 0; \
        irq_save(__critical_zone_eflags_##x);


#define EXIT_CRITICAL_ZONE(x) \
        irq_restore(__critical_zone_eflags_##x);

#define IRQ_CLOCK 0x00
#define IRQ_KEYBOARD 0x01
#define IRQ_CASCADE 0x02
#define IRQ_DISK 0x0E

#endif  //_IRQ_H

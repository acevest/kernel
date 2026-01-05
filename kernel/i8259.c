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

#include "i8259.h"

#include "io.h"
#include "irq.h"

void mask_i8259() {
    // mask all of 8259
    outb_p(0xFF, PIC_MASTER_IMR);
    outb_p(0xFF, PIC_SLAVE_IMR);
}

void init_i8259() {
    // Master...
    outb_p(0x11, PIC_MASTER_CMD);                    // ICW1
    outb_p(0x20, PIC_MASTER_IMR);                    // ICW2: IR0-7 mapped to 0x20-0x27
    outb_p((1U << PIC_CASCADE_IR), PIC_MASTER_IMR);  // IR2 Connect Slave.
#if PIC_AEOI
    outb_p(0x03, PIC_MASTER_IMR);
#else
    outb_p(0x01, PIC_MASTER_IMR);  // Normal EOI
#endif

    // Slave...
    outb_p(0x11, PIC_SLAVE_CMD);
    outb_p(0x28, PIC_SLAVE_IMR);  // IR0-7 mapped to 0x28-0x2F
    outb_p(PIC_CASCADE_IR, PIC_SLAVE_IMR);
#if PIC_AEOI
    outb_p(0x03, PIC_SLAVE_IMR);
#else
    outb_p(0x01, PIC_SLAVE_IMR);
#endif
    mask_i8259();
}

int enable_i8259_irq(unsigned int irq) {
    unsigned char mask = 0;
    if (irq & 8) {
        mask = ~(1 << (irq - 8));
        mask &= inb(PIC_SLAVE_IMR);
        outb(mask, PIC_SLAVE_IMR);
    } else {
        mask = ~(1 << irq);
        mask &= inb(PIC_MASTER_IMR);
        outb_p(mask, PIC_MASTER_IMR);
    }
}

int disable_i8259_irq(unsigned int irq) {
    unsigned char mask = 0;
    if (irq & 8) {
        mask |= (1 << (irq - 8));
        mask |= inb(PIC_SLAVE_IMR);
        outb(mask, PIC_SLAVE_IMR);
    } else {
        mask |= (1 << irq);
        mask |= inb(PIC_MASTER_IMR);
        outb(mask, PIC_MASTER_IMR);
    }
}

void mask_ack_i8259_irq(unsigned int irq) {
    unsigned char mask = 0;

    if (irq & 8)  // Slave
    {
        mask |= (1 << (irq - 8));
        mask |= inb(PIC_SLAVE_IMR);
        // Mask
        outb(mask, PIC_SLAVE_IMR);
#if PIC_AEOI
        // ...
#else
        // Specific EOI to slave
        outb(0x20 + (irq & 0x07), PIC_SLAVE_CMD);
        // Specific EOI to master
        outb(0x20 + (PIC_CASCADE_IR & 0x07), PIC_MASTER_CMD);
#endif
    } else  // Master
    {
        mask |= (1 << irq);
        mask |= inb(PIC_MASTER_IMR);
        // Mask
        outb(mask, PIC_MASTER_IMR);
#if PIC_AEOI
        // ...
#else
        // Specific EOI to master
        outb(0x20 + irq, PIC_MASTER_CMD);
#endif
    }
}

void ack_i8259_irq(unsigned int irq) {
    unsigned char mask = 0;

    if (irq & 8)  // Slave
    {
#if PIC_AEOI
        // ...
#else
        // Specific EOI to slave
        outb(0x20 + (irq & 0x07), PIC_SLAVE_CMD);
        // Specific EOI to master
        outb(0x20 + (PIC_CASCADE_IR & 0x07), PIC_MASTER_CMD);
#endif
    } else  // Master
    {
#if PIC_AEOI
        // ...
#else
        // Specific EOI to master
        outb(0x20 + irq, PIC_MASTER_CMD);
#endif
    }
}

irq_chip_t i8259_chip = {
    .name = "XT-PIC",
    .enable = enable_i8259_irq,
    .disable = disable_i8259_irq,
    .ack = ack_i8259_irq,
};

void do_i8259_IRQ(pt_regs_t* regs, unsigned int irq) {
}

__attribute__((regparm(1))) void boot_irq_handler(pt_regs_t* regs) {
    unsigned int irq = regs->irq;
    if (irq != 0 && irq != 1) {
        panic("invalid irq %d\n", irq);
    }

    assert(irq_disabled());

    // 屏蔽当前中断
    disable_i8259_irq(irq);

    // 发送EOI
    ack_i8259_irq(irq);

    // 解除屏蔽当前中断
    enable_i8259_irq(irq);
}

// IMCR
#define IMCR_ADDR_PORT 0x22
#define IMCR_DATA_PORT 0x23
// IMCR寄存器位定义
#define IMCR_REG_SELECT 0x70  // 选择寄存器70h
//
#define IMCR_BIT_APIC 0x01          // bit0: 1=APIC模式, 0=PIC模式
#define IMCR_BIT_PIC_MASK 0x02      // bit1: 0=启用PIC, 1=禁用PIC
#define IMCR_BIT_IMCR_PRESENT 0x80  // bit7: 1=IMCR存在, 0=IMCR不存在
uint8_t imcr_read(uint8_t reg) {
    outb(IMCR_ADDR_PORT, reg);
    return inb(IMCR_DATA_PORT);
}
void imcr_write(uint8_t reg, uint8_t value) {
    outb(IMCR_ADDR_PORT, reg);
    outb(IMCR_DATA_PORT, value);
}

void imcr_enable_apic_disable_pic() {
    uint8_t value = imcr_read(IMCR_REG_SELECT);
    printk("IMCR: %02x\n", value);
    if ((value & IMCR_BIT_IMCR_PRESENT) == 0) {
        panic("IMCR not present\n");
    }
    imcr_write(IMCR_REG_SELECT, value | IMCR_BIT_APIC | IMCR_BIT_PIC_MASK);
    value = imcr_read(IMCR_REG_SELECT);
    printk("IMCR: %02x\n", value);

    if ((value & IMCR_BIT_APIC) == 0) {
        panic("IMCR not set to APIC mode\n");
    } else {
        printk("IMCR set to APIC mode\n");
    }
}

void disable_i8259() {
    mask_i8259();

    imcr_enable_apic_disable_pic();

    // 禁用8259级联
    outb(0x11, PIC_MASTER_CMD);  // ICW1: 初始化
    outb(0x20, PIC_MASTER_IMR);  // ICW2: IR0-7 mapped to 0x20-0x27
    outb(0x04, PIC_MASTER_IMR);  // ICW3: IR2连接从片
    outb(0x01, PIC_MASTER_IMR);  // ICW4: Normal EOI
    outb(0x11, PIC_SLAVE_CMD);   // ICW1: 初始化
    outb(0x28, PIC_SLAVE_IMR);   // ICW2: IR0-7 mapped to 0x28-0x2F
    outb(0x02, PIC_SLAVE_IMR);   // ICW3: IR2连接从片
    outb(0x01, PIC_SLAVE_IMR);   // ICW4: Normal EOI

    mask_i8259();
}

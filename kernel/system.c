/*
 *--------------------------------------------------------------------------
 *   File Name: system.c
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Wed Mar  4 21:34:47 2009
 * Last Update: Wed Mar  4 21:34:47 2009
 *
 *--------------------------------------------------------------------------
 */

#include <assert.h>
#include <fs.h>
#include <i8259.h>
#include <ide.h>
#include <irq.h>
#include <page.h>
#include <processor.h>
#include <sched.h>
#include <string.h>
#include <syscall.h>
#include <system.h>

System system;
TSS_t tss;
Desc idt[NIDT] __attribute__((__aligned__(8)));
Desc gdt[NGDT] __attribute__((__aligned__(8)));
char gdtr[6] __attribute__((__aligned__(4)));
char idtr[6] __attribute__((__aligned__(4)));

extern char _gdtr[6];
extern char _idtr[6];

void setup_gdt() {
    pDesc pdesc;
    // change to new gdt.
    asm volatile("sgdt gdtr");

    // 复制旧的GDT
    memcpy(gdt, (void *)pa2va(*((uint32_t *)(gdtr + 2))), *((uint16_t *)(gdtr + 0)));
    *((unsigned short *)gdtr) = NGDT * sizeof(Desc);
    *((unsigned long *)(gdtr + 2)) = (unsigned long)gdt;

    asm volatile("lgdt gdtr");

    memcpy(gdt + INDEX_UCODE, gdt + INDEX_KCODE, sizeof(Desc));
    memcpy(gdt + INDEX_UDATA, gdt + INDEX_KDATA, sizeof(Desc));
    pdesc = gdt + INDEX_UCODE;
    pdesc->seg.DPL = 3;
    pdesc = gdt + INDEX_UDATA;
    pdesc->seg.DPL = 3;
}

// 中断门和陷阱门的区别是
// 通过中断门进入中断服务程序CPU会自动将中断关闭，也就是将EFLAGS的IF位清0
// 通过陷阱门进入服务程序时则维持IF标志位不变
void setup_idt() {
    *((unsigned short *)idtr) = NIDT * sizeof(Gate);
    *((unsigned long *)(idtr + 2)) = (unsigned long)idt;
    asm volatile("lidt idtr");
}

void setup_gates() {
    set_sys_int(0x00, TRAP_GATE, PRIVILEGE_KRNL, DivideError);
    set_sys_int(0x01, TRAP_GATE, PRIVILEGE_KRNL, Debug);
    set_sys_int(0x02, INTR_GATE, PRIVILEGE_KRNL, NMI);
    set_sys_int(0x03, TRAP_GATE, PRIVILEGE_USER, BreakPoint);
    set_sys_int(0x04, TRAP_GATE, PRIVILEGE_USER, OverFlow);
    set_sys_int(0x05, TRAP_GATE, PRIVILEGE_USER, BoundsCheck);
    set_sys_int(0x06, TRAP_GATE, PRIVILEGE_KRNL, InvalidOpcode);
    set_sys_int(0x07, TRAP_GATE, PRIVILEGE_KRNL, DeviceNotAvailable);
    set_sys_int(0x08, TRAP_GATE, PRIVILEGE_KRNL, DoubleFault);
    set_sys_int(0x09, TRAP_GATE, PRIVILEGE_KRNL, CoprocSegOverRun);
    set_sys_int(0x0A, TRAP_GATE, PRIVILEGE_KRNL, InvalidTss);
    set_sys_int(0x0B, TRAP_GATE, PRIVILEGE_KRNL, SegNotPresent);
    set_sys_int(0x0C, TRAP_GATE, PRIVILEGE_KRNL, StackFault);
    set_sys_int(0x0D, TRAP_GATE, PRIVILEGE_KRNL, GeneralProtection);
    set_sys_int(0x0E, TRAP_GATE, PRIVILEGE_KRNL, PageFault);
    set_sys_int(0x10, TRAP_GATE, PRIVILEGE_KRNL, CoprocError);

    for (int i = 0x11; i < 0x20; i++) {
        set_sys_int(i, INTR_GATE, PRIVILEGE_KRNL, no_irq_handler);
    }

    for (int i = 0x20; i < 256; i++) {
        set_sys_int(i, INTR_GATE, PRIVILEGE_KRNL, no_irq_handler);
    }
}

void ide_irq();

void default_irq_handler(unsigned int irq, pt_regs_t *regs, void *dev_id) { printk("default irq handler %d \n", irq); }

void init_i8259();

void setup_irqs() {
    set_sys_int(0x20, INTR_GATE, PRIVILEGE_KRNL, irq_0x00_handler);
    set_sys_int(0x21, INTR_GATE, PRIVILEGE_KRNL, irq_0x01_handler);
    set_sys_int(0x22, INTR_GATE, PRIVILEGE_KRNL, irq_0x02_handler);
    set_sys_int(0x23, INTR_GATE, PRIVILEGE_KRNL, irq_0x03_handler);
    set_sys_int(0x24, INTR_GATE, PRIVILEGE_KRNL, irq_0x04_handler);
    set_sys_int(0x25, INTR_GATE, PRIVILEGE_KRNL, irq_0x05_handler);
    set_sys_int(0x26, INTR_GATE, PRIVILEGE_KRNL, irq_0x06_handler);
    set_sys_int(0x27, INTR_GATE, PRIVILEGE_KRNL, irq_0x07_handler);
    set_sys_int(0x28, INTR_GATE, PRIVILEGE_KRNL, irq_0x08_handler);
    set_sys_int(0x29, INTR_GATE, PRIVILEGE_KRNL, irq_0x09_handler);
    set_sys_int(0x2A, INTR_GATE, PRIVILEGE_KRNL, irq_0x0A_handler);
    set_sys_int(0x2B, INTR_GATE, PRIVILEGE_KRNL, irq_0x0B_handler);
    set_sys_int(0x2C, INTR_GATE, PRIVILEGE_KRNL, irq_0x0C_handler);
    set_sys_int(0x2D, INTR_GATE, PRIVILEGE_KRNL, irq_0x0D_handler);
    set_sys_int(0x2E, INTR_GATE, PRIVILEGE_KRNL, irq_0x0E_handler);
    set_sys_int(0x2F, INTR_GATE, PRIVILEGE_KRNL, irq_0x0F_handler);

    for (int i = 0; i < NR_IRQS; i++) {
        irq_desc[i] = no_irq_desc;

        if (i < 16) {
            irq_desc[i].chip = &i8259_chip;
        }
    }

    void kbd_handler(unsigned int irq, pt_regs_t *regs, void *dev_id);
    void clk_handler(unsigned int irq, pt_regs_t *regs, void *dev_id);
    void clk_bh_handler();

    request_irq(0x00, clk_handler, "Intel 8254", "Clock Chip");
    request_irq(0x01, kbd_handler, "Intel 8042", "PS/2 Keyboard");
    // request_irq(0x0E, default_ide_irq_handler, "hard", "IDE");
    for (int i = 0; i < 16; i++) {
        if (i != 0 && i != 1 && i != 10 && i != 14 && i != 15) {
            request_irq(i, default_irq_handler, "default", "default");
        }
    }

    // 先关闭所有中断
    for (int i = 0; i < 16; i++) {
        close_irq(i);
    }

    // 清除8259A的级连中断引脚的中断屏蔽位
    // 以让从片的中断在放开后能发送到CPU
    open_irq(IRQ_CASCADE);

    // 打开支持的中断
    open_irq(IRQ_CLOCK);
    open_irq(IRQ_KEYBOARD);
    open_irq(IRQ_DISK);
    open_irq(15);
}

void boot_irq_handler();
void setup_boot_irqs() {
    init_i8259();

    // clock
    set_sys_int(0x20 + IRQ_CLOCK, INTR_GATE, PRIVILEGE_KRNL, _boot_clk_irq_handler);

    // keyboard
    set_sys_int(0x20 + IRQ_KEYBOARD, INTR_GATE, PRIVILEGE_KRNL, _boot_kbd_irq_handler);

    // 打开支持的中断
    enable_i8259_irq(IRQ_CLOCK);
    enable_i8259_irq(IRQ_KEYBOARD);
}

void set_tss_gate(u32 vec, u32 addr, u32 limit) {
    pSeg p = (pSeg)(gdt + vec);
    _init_desc((pDesc)p);
    p->limitL = 0xFFFF & limit;
    p->limitH = 0x0F & (limit >> 16);
    p->baseL = 0xFFFF & addr;
    p->baseM = 0xFF & (addr >> 16);
    p->baseH = 0xFF & (addr >> 24);

    p->P = 1;
    p->DPL = PRIVILEGE_USER;
    p->S = 0;

    p->type = TSS_DESC;
}

void set_tss() {
    TSS_t *p = &tss;
    memset((void *)p, sizeof(TSS_t), 0);
    p->esp0 = 0;  // delay to init root_task
    p->ss0 = SELECTOR_KRNL_DS;
    p->ss = SELECTOR_KRNL_DS;
    p->gs = SELECTOR_KRNL_DS;
    p->fs = SELECTOR_KRNL_DS;
    p->es = SELECTOR_KRNL_DS;
    p->ds = SELECTOR_KRNL_DS;
    p->cs = SELECTOR_KRNL_CS;
    p->eflags = 0x1200;
    p->iomap_base = sizeof(TSS_t);

    // tss的iomap_base=sizeof(TSS_t) > TSS_GATE.LIMIT - 1
    // 因此表示没有I/O位图
    set_tss_gate(INDEX_TSS, (u32)p, sizeof(TSS_t));

    asm("ltr %%ax" ::"a"((INDEX_TSS << 3) + 3));
}

int sysc_reboot(int mode) {
    void do_reboot();
    void do_poweroff();

    switch (mode) {
    case 0:
        do_reboot();
        break;
    case 1:
        do_poweroff();
        break;
    }

    return 0;
}

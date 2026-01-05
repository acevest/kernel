/*
 * ------------------------------------------------------------------------
 *   File Name: ap.c
 *      Author: Zhao Yanbai
 *              2026-01-04 20:15:33 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <page.h>
#include <types.h>
#include <system.h>
#include <processor.h>
#include <linkage.h>

extern pde_t* ap_pre_pgd;

#define AP_GDT_CNT 8
#define AP_IDT_CNT 256

desc_t ALIGN8 ap_idt[AP_IDT_CNT];
uint8_t ALIGN8 ap_idtr[6];

#define AP_CS_SELECTOR 0x08
#define AP_DS_SELECTOR 0x10

static inline desc_t _create_ap_gate(u32 handler, u8 type, u8 DPL) {
    desc_t d;
    gate_t* p = &d.gate;

    _init_desc(&d);

    p->eaddrL = 0xFFFF & handler;
    p->eaddrH = 0xFFFF & (handler >> 16);
    p->selector = AP_CS_SELECTOR;
    p->type = type;
    p->P = 0x1;
    p->DPL = DPL;

    return d;
}

static inline void set_ap_idt_gate(u32 vec, u32 handler, u8 type, u8 DPL) {
    ap_idt[vec] = _create_ap_gate(handler, type, DPL);
}

#define set_ap_sys_int(vect, type, DPL, handler)        \
    do {                                                \
        void handler();                                 \
        set_ap_idt_gate(vect, (u32)handler, type, DPL); \
    } while (0)

void ap_kernel_entry() {
    // 虽然ap_pre_pgd只做了一下跳板页目录看起来很可惜
    // 不过可以把它拿来个AP的栈用
    asm("mov %0, %%esp" : : "r"(pa2va(&ap_pre_pgd) + PAGE_SIZE));

    // 进入内核空间后要加载原来已经在内核空间的ap_gdt
    // 之前加载的被复制到1MB以下的ap_gdt需要废弃，因为它已经在内核地址空间之外了
    // 虽然他们是同一个ap_gdt
    extern char ap_gdtr;
    asm("lgdt ap_gdtr");

    // tss
    // AP 暂时不服务用户特权级 所以暂时用不到tss

    //
    set_ap_sys_int(0x00, TRAP_GATE, PRIVILEGE_KRNL, APDivideError);
    set_ap_sys_int(0x01, TRAP_GATE, PRIVILEGE_KRNL, APDebug);
    set_ap_sys_int(0x02, INTR_GATE, PRIVILEGE_KRNL, APNMI);
    set_ap_sys_int(0x03, TRAP_GATE, PRIVILEGE_USER, APBreakPoint);
    set_ap_sys_int(0x04, TRAP_GATE, PRIVILEGE_USER, APOverFlow);
    set_ap_sys_int(0x05, TRAP_GATE, PRIVILEGE_USER, APBoundsCheck);
    set_ap_sys_int(0x06, TRAP_GATE, PRIVILEGE_KRNL, APInvalidOpcode);
    set_ap_sys_int(0x07, TRAP_GATE, PRIVILEGE_KRNL, APDeviceNotAvailable);
    set_ap_sys_int(0x08, TRAP_GATE, PRIVILEGE_KRNL, APDoubleFault);
    set_ap_sys_int(0x09, TRAP_GATE, PRIVILEGE_KRNL, APCoprocSegOverRun);
    set_ap_sys_int(0x0A, TRAP_GATE, PRIVILEGE_KRNL, APInvalidTss);
    set_ap_sys_int(0x0B, TRAP_GATE, PRIVILEGE_KRNL, APSegNotPresent);
    set_ap_sys_int(0x0C, TRAP_GATE, PRIVILEGE_KRNL, APStackFault);
    set_ap_sys_int(0x0D, TRAP_GATE, PRIVILEGE_KRNL, APGeneralProtection);
    set_ap_sys_int(0x0E, TRAP_GATE, PRIVILEGE_KRNL, APPageFault);
    set_ap_sys_int(0x10, TRAP_GATE, PRIVILEGE_KRNL, APCoprocError);

    // idt gates
    for (int i = 0x11; i < 0x20; i++) {
        set_ap_sys_int(i, INTR_GATE, PRIVILEGE_KRNL, ap_no_irq_handler);
    }

    // irq gates
    for (int i = 0x20; i < 256; i++) {
        set_ap_sys_int(i, INTR_GATE, PRIVILEGE_KRNL, ap_no_irq_handler);
    }

    // 加载 ap_idtr
    *((unsigned short*)ap_idtr) = AP_IDT_CNT * sizeof(gate_t);
    *((unsigned long*)(ap_idtr + 2)) = (unsigned long)ap_idt;
    asm volatile("lidt ap_idtr");

    asm("int $0x28");

    while (1) {
        asm("nop;");
        asm("int $0x28");
    }
}

void do_ap_no_irq_handler() {
    // do nothing
    // printk("AP no irq handler\n");
    uint8_t* p = (uint8_t*)0xC00B8000;
    *p = *p == ' ' ? 'K' : ' ';
}

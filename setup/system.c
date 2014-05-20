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

#include <system.h>
#include <string.h>
#include <processor.h>
#include <page.h>
#include <irq.h>
#include <sched.h>
#include <assert.h>
#include <i8259.h>
#include <fs.h>

void    setup_gdt()
{
    pDesc    pdesc;
    //change to new gdt.
    sgdt();
    memcpy(gdt, (void *)pa2va(*((unsigned long*)(gdtr+2))),
                *((unsigned short *)gdtr));
    *((unsigned short *)gdtr)    = NGDT*sizeof(Desc);
    *((unsigned long  *)(gdtr+2))    = (unsigned long)gdt;
    lgdt();
    memcpy(gdt+INDEX_UCODE, gdt+INDEX_KCODE, sizeof(Desc));
    memcpy(gdt+INDEX_UDATA, gdt+INDEX_KDATA, sizeof(Desc));
    pdesc = gdt+INDEX_UCODE;
    pdesc->seg.DPL = 3;
    pdesc = gdt+INDEX_UDATA;
    pdesc->seg.DPL = 3;
}

void    setup_idt()
{
    //init idt.
    *((unsigned short *)idtr)    = NIDT*sizeof(Gate);
    *((unsigned long  *)(idtr+2))    = (unsigned long)idt;
    lidt();
}

void    setup_gate()
{    int i;
#define set_sys_int(vect, type, DPL, handler)        \
do{                            \
    extern    void    handler ();            \
    set_idt_gate(vect, (u32)handler, type, DPL);    \
}while(0)

#define PL_KRNL    PRIVILEGE_KRNL
#define PL_USER    PRIVILEGE_USER
#if 1
    set_sys_int(0x00, TRAP_GATE, PL_KRNL, DivideError);
    set_sys_int(0x01, TRAP_GATE, PL_KRNL, Debug);
    set_sys_int(0x02, INTR_GATE, PL_KRNL, NMI);    
    set_sys_int(0x03, TRAP_GATE, PL_USER, BreakPoint);    
    set_sys_int(0x04, TRAP_GATE, PL_USER, OverFlow);    
    set_sys_int(0x05, TRAP_GATE, PL_USER, BoundsCheck);    
    set_sys_int(0x06, TRAP_GATE, PL_KRNL, InvalidOpcode);    
    set_sys_int(0x07, TRAP_GATE, PL_KRNL, DeviceNotAvailable);    
    set_sys_int(0x08, TRAP_GATE, PL_KRNL, DoubleFault);    
    set_sys_int(0x09, TRAP_GATE, PL_KRNL, CoprocSegOverRun);    
    set_sys_int(0x0A, TRAP_GATE, PL_KRNL, InvalidTss);    
    set_sys_int(0x0B, TRAP_GATE, PL_KRNL, SegNotPresent);    
    set_sys_int(0x0C, TRAP_GATE, PL_KRNL, StackFault);    
    set_sys_int(0x0D, TRAP_GATE, PL_KRNL, GeneralProtection);    
    set_sys_int(0x0E, TRAP_GATE, PL_KRNL, PageFault);    
    set_sys_int(0x10, TRAP_GATE, PL_KRNL, CoprocError);

    for(i=0x20; i<256; i++)
        set_sys_int(i, INTR_GATE, PL_KRNL, no_irq_handler);

#if 1
    set_sys_int(0x20, INTR_GATE, PL_KRNL, irq_0x00_handler);
    set_sys_int(0x21, INTR_GATE, PL_KRNL, irq_0x01_handler);    
    set_sys_int(0x22, INTR_GATE, PL_KRNL, irq_0x02_handler);    
    set_sys_int(0x23, INTR_GATE, PL_KRNL, irq_0x03_handler);    
    set_sys_int(0x24, INTR_GATE, PL_KRNL, irq_0x04_handler);    
    set_sys_int(0x25, INTR_GATE, PL_KRNL, irq_0x05_handler);    
    set_sys_int(0x26, INTR_GATE, PL_KRNL, irq_0x06_handler);    
    set_sys_int(0x27, INTR_GATE, PL_KRNL, irq_0x07_handler);    
    set_sys_int(0x28, INTR_GATE, PL_KRNL, irq_0x08_handler);    
    set_sys_int(0x29, INTR_GATE, PL_KRNL, irq_0x09_handler);    
    set_sys_int(0x2A, INTR_GATE, PL_KRNL, irq_0x0A_handler);    
    set_sys_int(0x2B, INTR_GATE, PL_KRNL, irq_0x0B_handler);    
    set_sys_int(0x2C, INTR_GATE, PL_KRNL, irq_0x0C_handler);    
    set_sys_int(0x2D, INTR_GATE, PL_KRNL, irq_0x0D_handler);    
    set_sys_int(0x2E, INTR_GATE, PL_KRNL, irq_0x0E_handler);    
    set_sys_int(0x2F, INTR_GATE, PL_KRNL, irq_0x0F_handler);    
#endif
#endif
}

void    setup_irqs()
{
    extern    void init_i8259();
    init_i8259();

    int i;
    for(i=0; i<NR_IRQS; i++)
    {
        irq_desc[i] = no_irq_desc;

        if(i<16)
            irq_desc[i].chip = &i8259_chip;
    }

    //extern    void kbd_handler(pt_regs_t *, unsigned int); //extern    void clk_handler(pt_regs_t *, unsigned int);
    //extern    void hd_handler(pt_regs_t *, unsigned int);
    void    kbd_handler(unsigned int irq, pt_regs_t * regs, void *dev_id);
    void    clk_handler(unsigned int irq, pt_regs_t * regs, void *dev_id);
    void    hd_handler(unsigned int irq, pt_regs_t * regs, void *dev_id);
    request_irq(0x00, clk_handler,    "Intel 8254",    "Clock Chip");
    request_irq(0x01, kbd_handler,    "Intel 8042",    "PS/2 Keyboard");
    //request_irq(0x0E, hd_handler,     "IDE",           "IDE");
    for(i=2; i<16; i++)
    {
        request_irq(i, hd_handler,     "IDE",           "IDE");
    }

    enable_irq(0x00);
    enable_irq(0x01);
    enable_irq(0x02);
    enable_irq(0x03);
    enable_irq(0x04);
    enable_irq(0x05);
    enable_irq(0x06);
    enable_irq(0x07);
    enable_irq(0x08);
    enable_irq(0x09);
    enable_irq(0x0A);
    enable_irq(0x0B);
    enable_irq(0x0C);
    enable_irq(0x0D);
    enable_irq(0x0F);
    enable_irq(0x0E);
    asm("sti");
    //asm("cli");

/*
    pIRQAction    pKbdAction, pClkAction;


    pKbdAction    = (pIRQAction) kmalloc_old(sizeof(IRQAction));
    pClkAction    = (pIRQAction) kmalloc_old(sizeof(IRQAction));


    assert(pKbdAction != NULL);
    assert(pClkAction != NULL);

    pClkAction->handler    = clk_handler;
    pClkAction->next    = NULL;
    pKbdAction->handler    = kbd_handler;
    pKbdAction->next    = NULL;

    printk("**********%08x %08x", pKbdAction, pClkAction);

    irq_desc[0x0].chip = &i8259_chip;
    irq_desc[0x0].chip->enable(0x0);
    irq_desc[0x0].action = pClkAction;

    irq_desc[0x1].chip = &i8259_chip;
    irq_desc[0x1].chip->enable(0x1);
    irq_desc[0x1].action = pKbdAction;
*/

}

void    set_tss()
{
    pTSS p = &tss;
    memset((void *)p, sizeof(TSS), 0);
    p->esp0      = 0; // delay to init root_task
    p->ss0       = SELECTOR_KRNL_DS;
    p->ss        = SELECTOR_KRNL_DS;
    p->gs        = SELECTOR_KRNL_DS;
    p->fs        = SELECTOR_KRNL_DS;
    p->es        = SELECTOR_KRNL_DS;
    p->ds        = SELECTOR_KRNL_DS;
    p->cs        = SELECTOR_KRNL_CS;
    p->eflags    = 0x1200;
    p->iomap_base    = sizeof(TSS);
    set_tss_gate(INDEX_TSS, (u32)p);
    //printk("TSS:%08x\n", p);
    asm("ltr %%ax"::"a"((INDEX_TSS<<3)+3));
}

void setup_root_dev()
{
    unsigned char dev;
    dev = (unsigned char)(system.boot_device >> 24);
    //if(dev != 0x80)
    //    panic("OS must boot from the first hard disk");

    printk("root device: %08x\n", system.root_dev);
#if 0

    /* 
     * 硬盘的次设备号
     * 0: 整个硬盘
     * 1-4: 主分区
     * 5~N: 逻辑分区(N<2^16-5)
     * 目录只支持从第一个硬盘引导.
     */
    int    minor = ((system.boot_device>>16) & 0xFF) + 1;
    system.root_dev = MAKE_DEV(DEV_MAJOR_HD, minor);
#endif
}

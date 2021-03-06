/*
 *--------------------------------------------------------------------------
 *   File Name: interrupts.S
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Thu Jul 16 18:54:08 2009
 * Last Update: Wed Feb 10 23:10:56 2010
 * 
 *--------------------------------------------------------------------------
 */
#define ASM
#include <linkage.h>
#include <system.h>
#include <task.h>

#define DEF_IRQ(a,b)            \
.align  0x04,0x90;              \
.global irq_0x##a##b##_handler; \
irq_0x##a##b##_handler:         \
pushl   $0x##a##b;              \
jmp     _irq_handler;

.align  0x04,0x90
.global no_irq_handler
no_irq_handler:
pushl   $0xFFFFFFFF
jmp     _irq_handler


DEF_IRQ(0,0)
DEF_IRQ(0,1)
DEF_IRQ(0,2)
DEF_IRQ(0,3)
DEF_IRQ(0,4)
DEF_IRQ(0,5)
DEF_IRQ(0,6)
DEF_IRQ(0,7)
DEF_IRQ(0,8)
DEF_IRQ(0,9)
DEF_IRQ(0,A)
DEF_IRQ(0,B)
DEF_IRQ(0,C)
DEF_IRQ(0,D)
DEF_IRQ(0,E)
DEF_IRQ(0,F)


.global _irq_handler
.extern irq_handler
.extern schedule
_irq_handler:
    SAVE_REGS

    movw    %ss, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %fs
    movw    %ax, %gs

    movl    %esp, %eax
    call    irq_handler

    movl    $-TASK_SIZE, %ebp
    andl    %esp, %ebp

    movl    PT_REGS_CS(%esp), %eax
    testl   $0x0003, %eax
    jz      resume_kernel

return:
    cmpl    $0, TI_preempt_cnt(%ebp)
    jnz     restore_regs

resched:

    call    schedule

restore_regs:

    RESTORE_REGS
    
    addl    $4, %esp

    iret

resume_kernel:
    # ... 
    jmp     return

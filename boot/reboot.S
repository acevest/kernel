/*
 *--------------------------------------------------------------------------
 *   File Name: reboot.S
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Sat Jul 11 19:16:42 2009
 * Last Update: Sat Jul 11 19:16:42 2009
 * 
 *--------------------------------------------------------------------------
 */
#define ASM
#include <system.h>
.global do_restart
.global do_reboot
.global do_poweroff
.extern printk
.text
.code32
.align 32
do_restart:
do_reboot:
    movl    $REBOOT_RESTART,reboot_flag
    pushl    $msg_reboot
    call    printk
    addl    $4,%esp
    jmp    _do_reboot

do_poweroff:
    movl    $REBOOT_POWEROFF,reboot_flag
    pushl    $msg_poweroff
    call    printk
    addl    $4,%esp
    jmp    _do_reboot


_do_reboot:

    # 刚打印完提示字符串，先等待3秒钟
    sti
    movl    $300, %ecx
 .delay:
    hlt
    loop .delay


    # Disable All Interrupts
    cli

    # Write to CMOS
    movb    $0x8F,%al
    movw    $0x70,%dx
    outb    %al,%dx
    movb    $0x00,%al
    movw    $0x71,%dx
    outb    %al,%dx


    # Prepare Paging For Reboot 
    movl    $256,%ecx
    movl    $0x100C00,%esi
    movl    $0x100000,%edi
    rep
    movsl


    # Modify 'jmp' Instruction In Real Mode Code
    cmpl    $REBOOT_POWEROFF,reboot_flag
    je    1f
    cmpl    $REBOOT_RESTART,reboot_flag
    je    2f
1:
    movl    $JUMPEND,%ecx
    subl    $JUMPBEGIN,%ecx
    movl    $JUMPBEGIN,%edi
    movb    $0x90,%al
    rep
    stosb

2:
    # Copy The Real Mode Code to Low Memory
    movl    $Code16End,%ecx
    subl    $Code16,%ecx
    movl    $Code16,%esi
    movl    $0x1000,%edi
    rep
    movsb

    # Reload CR3
    #movl    $0x100000,%eax
    #movl    %eax,%cr3

    # Set Reboot Mode
    # 0x1234 For Warm Reboot
    # 0x0000 For Cold Reboot
    movw    $0x1234,(0x472)

    lidt    IDTR
    lgdt    GDTR

    movl    $0x10,%eax
    movw    %ax,%ds
    movw    %ax,%es
    movw    %ax,%fs
    movw    %ax,%gs
    movw    %ax,%ss

    # Jump to Real Mode Code
    ljmp    $0x0008,$0x1000

RebootGDT:
EMPT:    .long    0x00000000,0x00000000
Code:    .long    0x0000FFFF,0x00009A00
Data:    .long    0x0000FFFF,0x00009200
RebootGDTEnd:
GDTR:
    GDTRLimit:    .word    RebootGDTEnd-RebootGDT
    GDTRBase:    .long    RebootGDT
IDTR:
    IDTRLimit:    .word    0x3FF
    IDTRBase:    .long    0x0
reboot_flag:    .long    REBOOT_RESTART
.align    16
.code16
Code16:
    movl    %cr0,%eax
    andl    $0x00000011,%eax
    orl    $0x60000000,%eax
    movl    %eax,%cr0
    movl    %eax,%cr3
    movl    %cr0,%ebx
    andl    $0x60000000,%ebx
    jz    1f
    invd
1:
    andb    $0x10,%al
    movl    %eax,%cr0

    # The Jump Instruction Will be Set 'nop' If Need Reboot.
JUMPBEGIN:
    jmp    REBOOT
JUMPEND:

SHUTDOWN:
    movw    $0x1000,%ax
    movw    %ax,%ss
    movw    $0xF000,%sp
    movw    $0x5307,%ax
    movw    $0x0001,%bx
    movw    $0x0003,%cx
    int    $0x15
REBOOT:
    ljmp    $0xFFFF,$0x0000
Code16End:


msg_reboot:    .asciz    "\nREBOOTING...........\n"
msg_poweroff:  .asciz    "\nSHUTDOWN SYSTEM NOW......\n"

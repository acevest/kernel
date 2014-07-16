/*
 *--------------------------------------------------------------------------
 *   File Name: syscall.S
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version: 1.0
 * Create Date: Tue Feb 10 13:20:33 2009
 *     Version: 2.0
 * Last Update: Wed Feb 10 23:36:34 2010
 *     Version: 3.0
 * Last Update: Fri May  2 00:21:37 2014
 * 
 *--------------------------------------------------------------------------
 */

#define ASM
#include <system.h>
#include <syscall.h>

.code32
.text
.global syscall_entry
.global ret_from_fork_user
.global ret_from_fork_krnl
.global sysexit

syscall_entry:
    movl    (%esp),%esp

    pushl   $(SELECTOR_USER_SS)
    pushl   %ebp
    pushfl
    pushl   $(SELECTOR_USER_CS)
    pushl   $sysexit
    pushl   $0

    SAVE_REGS

    pushl   %eax
    movw    %ss, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %gs
    movw    %ax, %fs
    popl    %eax

    cmpl    $SYSC_NUM, %eax
    jae     bad_sysc_nr

    call    *sysc_handler_table(,%eax,4)

syscall_exit:
    movl    %eax, PT_REGS_EAX(%esp)

    RESTORE_REGS

    # addl    $20, %esp # no need now
    
    leal    sysexit, %edx
    movl    %ebp, %ecx

    sti        /* sysenter have cleared IF, and sysexit will not set IF. */
    sysexit

bad_sysc_nr:
    call    sysc_bad_syscnr
    jmp     syscall_exit

ret_from_fork_user:
    xorl    %eax, %eax
    jmp     syscall_exit

ret_from_fork_krnl:
    movl    PT_REGS_EDX(%esp), %edx
    sti
    call    *%edx
    #call    do_exit

    
    

# this routine should be load align by PAGE_SIZE
# this page is shared by the kernel and userspace
# the reason why i use this method is because of that the user program
# no need to pay attention on the return address
.section .sysexit
.align 0x1000 
sysexit:
    popl    %ebp;
    popl    %edx;
    popl    %ecx;
    ret

.global ring3
ring3:
    nop;
    nop;

    movl $(ring3_stack+100), %esp

    movl $11, %eax;

    pushl $1f;
    pushl %ecx;
    pushl %edx;
    pushl %ebp;
    movl %esp,%ebp;
    sysenter;
    1:

    nop;
    nop;

    jmp ring3

ring3_stack:
    .byte   200

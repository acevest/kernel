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
#include <task.h>

.code32
.text
.global syscall_entry
.global ret_from_fork_user
.global ret_from_fork_krnl
.global sysexit
//.global syscall_exit

syscall_entry:
    // 这段代码纯是为了pt_regs_t准备的
    // 后续看看直接优化掉
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
    jb      .normal_syscall
    mov     $SYSC_BAD_NR, %eax

 .normal_syscall:
    call    *sysc_handler_table(,%eax,4)

// normal_syscall_exit:
//     leal    sysexit, %edx
//     movl    %edx, PT_REGS_EDX(%esp)
//     movl    PT_REGS_EBP(%esp), %edx
//     movl    %edx, PT_REGS_ECX(%esp)

//syscall_exit:

    // 保存返回值
    movl    %eax, PT_REGS_EAX(%esp)

ret_from_fork_user:

    RESTORE_REGS

    // 返回用户态的eip
    // sysexit指令从edx中恢复用户态eip
    leal    sysexit, %edx

    // 在系统调用前，用户态的esp保存在ebp中
    // sysexit指令从ecx中恢复用户态esp
    movl    %ebp, %ecx

    sti        /* sysenter have cleared IF, and sysexit will not set IF. */
    sysexit


// 内核线程fork出路
ret_from_fork_krnl:
    RESTORE_REGS
    addl    $24, %esp
    call    *%edx

    # 还没有实现do_exit
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
.global ring3_stack_top
ring3:
    nop;
    nop;

    #movl $(ring3_stack+100), %esp

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
    .long   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .long   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .long   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    .long   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
ring3_stack_top:
    .long   0x12345678

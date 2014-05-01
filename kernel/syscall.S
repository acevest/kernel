/*
 *--------------------------------------------------------------------------
 *   File Name: syscall.S
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Tue Feb 10 13:20:33 2009
 *     Version: 2.0
 * Last Update: Wed Feb 10 23:36:34 2010
 * 
 *--------------------------------------------------------------------------
 */

#define ASM
#include <system.h>
#include <syscall.h>

#define TASK_SIZE 4096*2
.code32
#if 0
.extern sysc_handler_table
#endif
.text
.global syscall_entry
.global ret_from_fork
.global sysexit

#if 1
syscall_entry:
    movl    (%esp),%esp

    SAVE_REGS

    pushfl
    pushl    %ebp
    pushl    %eax

    movw    %ss, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %gs
    movw    %ax, %fs

    popl    %eax

    cmpl    $SYSC_NUM, %eax
    jae    bad_syscnr

    call    *sysc_handler_table(,%eax,4)

ret_from_bad_syscnr:
    popl    %ebp
    popfl
    RESTORE_REGS
    
    leal    sysexit, %edx
    movl    %ebp, %ecx

    sti        /* sysenter会自动清除IF.貌似sysexit不会自动置位 */
    sysexit

ret_from_fork: /* for compiler now */
bad_syscnr:
    call    sysc_bad_syscnr
    jmp    ret_from_bad_syscnr

#else
syscall_entry:
    /* 此时%esp存的是current的地址(&current) */
    movl    (%esp),%esp        /* 获得current的值 */
    addl    $TASK_SIZE, %esp    /* 指向PCB的顶部 */

    pushl    %ds            /* %ss */
    pushl    %ebp            /* %esp */
    pushfl                /* %eflags */
    pushl    $SELECTOR_USER_CS    /* %cs */
    pushl    %gs:(,%ebp)        /* %eip */
    pushl    %eax            /* sysc_nr */
    SAVE_REGS

    pushl    %eax
    movw    %ss,%ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %gs
    movw    %ax, %fs
    popl    %eax

    cmpl    $SYSC_NUM, %eax
    jae    bad_syscnr

    call    *sysc_handler_table(,%eax,4)

ret_from_bad_syscnr:


    movl    %eax, 0x18(%esp)    /* return value */

    /*
     * fork 后的新进程的返回值已经在栈上被设定为0了.
     * ret_from_fork 就是新进程被换入执行的入口地址.
     */
ret_from_fork:

    RESTORE_REGS
    
    movl    0x04(%esp), %edx    /* sysexit 参数 %eip */
    movl    0x10(%esp), %ecx    /* sysexit 参数 %esp */

    #xchg    %bx, %bx
    sti        /* sysenter会自动清除IF.貌似sysexit不会自动置位 */
    sysexit

bad_syscnr:
    call    sysc_bad_syscnr
    jmp    ret_from_bad_syscnr
#endif


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
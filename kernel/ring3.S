/*
 * ------------------------------------------------------------------------
 *   File Name: ring3.S
 *      Author: Zhao Yanbai
 *              2025-12-30 22:12:25 Tuesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#define ASM
#include <syscall.h>

# a allocatable 可分配，表示这个节在程序运行时需要加载到内存中
# x executable  可执行，表示这个节包含可执行的代码
# w writable    可写，通常用于数据节
# r read-only   只读
# M mergeable   可合并相同内容
# S strings     包含以null结尾的字符串
# G group       属于某个节组
#
# @progbits     包含了程序数据  目标文件中实际包含数据/代码，如.text、.data节
# @nobits       不包含程序数据  目标文件中不占空间，运行时分配零初始化内存，如.bss节
.section .ring3.entry, "ax", @progbits
.align 0x1000
ring3_entry:
    nop
    nop
    nop

    movl    $SYSC_RAND, %eax
    call    ring3_syscall


    movl    %eax, %ecx
    andl    $0x000000FF, %ecx
    movl    $SYSC_WAIT, %eax
    call    ring3_syscall

    nop
    nop
    jmp ring3_entry

# eax 系统调用号
# ecx 参数1
ring3_syscall:
    pushl   %ebp

    pushl   %ebx
    pushl   %edi

    movl    %ecx, %ebx
    call    get_eip
get_eip:
    # 这里popl取得的就是上一个call指令自动压入栈中的地址
    # 这个地址就是当前指令的地址
    popl    %edi
    addl    $8,%edi
    movl    %esp, %ebp;

    sysenter

    popl    %edi
    popl    %ebx
    popl    %ebp

    ret

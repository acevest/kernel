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

    pushl   $0                  # 模拟一个内核不需要的参数1
    pushl   $SYSC_RAND          # 系统调用号
    call    ring3_syscall
    addl    $8, %esp


    movl    %eax, %ecx
    andl    $0x000000FF, %ecx
    pushl   %ecx                # 参数1
    pushl   $SYSC_WAIT          # 系统调用号
    call    ring3_syscall
    addl    $8, %esp


    # 纯消耗CPU时间
    movl    $0x0F0F0F0F, %ecx
1:
    loop    1b


    nop
    nop
    jmp ring3_entry

# 传入: 系统调用号, 参数1
ring3_syscall:
    pushl   %ebp
    movl    %esp, %ebp

    pushl   %ebx # 用于保存参数1
    pushl   %edi # 用于保存sysexit后返回用户态的地址
                 # ebp用于保存sysexit后的用户态栈，不过已经保存了，这里不用再保存

    movl    0x08(%ebp), %eax # 系统调用号
    movl    0x0C(%ebp), %ebx # 参数1

    # 获取call get_eip后的地址
    call    get_eip
get_eip:
    # 这里popl取得的就是上一个call指令自动压入栈中的地址
    # 这个地址就是当前指令的地址
    popl    %edi

    # 算出sysenter后的地地址
    addl    $8, %edi

    # 设置sysexit返回用户态后的栈位置
    movl    %esp, %ebp

    sysenter

    # 从栈中恢复保存的易失寄存器
    popl    %edi
    popl    %ebx

    #
    popl    %ebp

    ret

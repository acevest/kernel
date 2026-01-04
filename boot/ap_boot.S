/*
 * ------------------------------------------------------------------------
 *   File Name: ap_boot.S
 *      Author: Zhao Yanbai
 *              2026-01-04 08:18:07 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

.code16
#.text
#.section .ap_boot, "ax", @progbits
.global ap_boot_bgn
.global ap_boot_end
.align 0x1000
ap_boot_bgn:
ap_realmode_base:
ap_real_mode_entry:
    cli

    wbinvd # 将CPU缓存写回主内存并使缓存失效

    xorl    %ebx, %ebx

    movw    %cs, %bx
    movw    %bx, %ds
    movw    %bx, %es
    movw    %bx, %ss

    # 算出ap_realmode_base的线性地址
    shl     $4, %ebx

    movl    %ebx, %esp
    addl    $0x1000, %esp

    # 算出gdt的线性地址
    leal    (ap_gdt - ap_realmode_base)(%ebx), %eax
    # 把这个地址填入gdtr的base位置
    movl    %eax, (ap_gdtr_base - ap_realmode_base)


    # 算出ap_code32_entry的线性地址
    leal    (ap_code32_entry - ap_realmode_base)(%ebx), %eax
    # 把这个地址填入ap_code32_entry_address
    mov     %eax, (ap_code32_entry_address - ap_realmode_base)


    lgdt    ap_gdtr - ap_realmode_base

    # enable PE
    movl    %cr0,%eax
    orl     $0x00000001,%eax
    movl    %eax,%cr0

    # 清空指令流水线
    jmp     .flush
.flush:

    # ljmpl 的前后两个l都很重要
    ljmpl    *(ap_code32_entry_address - ap_realmode_base)


.code32
.align 32
ap_code32_entry:
    cli
    nop
    nop
    hlt


.align 32
ap_code32_entry_address:    .long 0x00000000 # 等待动态写入
                            .word 0x0008, 0x0000

.align 32
ap_gdt:
empty:     .long    0x00000000, 0x00000000
code_desc: .long    0x0000FFFF, 0x00CF9B00
data_desc: .long    0x0000FFFF, 0x00CF9300
ap_gdt_end:
ap_gdtr:
    ap_gdtr_limit: .word ap_gdt_end-ap_gdt
    ap_gdtr_base:  .long 0 # 等待动态写入

ap_boot_end:

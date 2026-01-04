/*
 *--------------------------------------------------------------------------
 *   File Name: multiboot.S
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Mon Nov 10 15:58:46 2008
 * Last Update: Tue Feb 10 22:39:24 2009
 *
 *--------------------------------------------------------------------------
 */

.global kernel_entry
.global main
.extern check_kernel
.extern init_system_info
.extern setup_kernel
.extern root_task_entry
.extern get_root_task_stack_top

#define MULTIBOOT_STACK_SIZE 0x200

.section .kernel_entry_text
kernel_entry:
main:
    cli
    jmp real_kernel_entry

#
# 这之间放multiboot2_header
#

.text
.code32
.align 4
real_kernel_entry:
    # Load GDT's Information To GDTR
    lgdt    boot_gdtr

    movw    $0x10, %dx
    movw    %dx,%ds
    movw    %dx,%es
    movw    %dx,%ss
    movw    %dx,%fs
    movw    %dx,%gs

    movl    $(stack + MULTIBOOT_STACK_SIZE), %esp

    # Reset EFLAGS
    pushl   $0
    popf

    # Save Multiboot Infomation...
    pushl   %eax
    addl    $kernel_virtual_addr_start,%ebx
    pushl   %ebx

    call boot_paging

    # enable PG WP
    movl    %cr0,%eax
    orl     $0x80010000,%eax
    movl    %eax,%cr0

    jmp     4f
4:
    # 准备切保护模式
    # 此时esp的地址还需要再调整一下
    addl    $kernel_virtual_addr_start, %esp
    .extern p_entry_vaddr
    ljmp    $0x08, $p_entry_vaddr

.align 4
.global p_entry # 为了让链接器计算出 p_entry_vaddr
                # 因为编译器不支持 ljmp $0x08, $p_entry + kernel_virtual_addr_start 这种写法
p_entry:
    leal    check_kernel, %eax
    call    *%eax

    leal    get_root_task_stack_top, %eax
    call    *%eax
    movl    %eax, %esp

    leal    init_system_info, %eax
    call    *%eax;

    leal    setup_kernel, %eax
    call    *%eax

    subl    $128, %esp   // ss esp eip
    leal    root_task_entry, %eax
    jmpl    *%eax

die:
    jmp     die    # Should never come to here.


.align 4
p_label_far_ptr:
    .long   0x00
    .word   0x08, 0x00

.align 32
boot_gdt:
empty:     .long    0x00000000, 0x00000000
code_desc: .long    0x0000FFFF, 0x00CF9B00
data_desc: .long    0x0000FFFF, 0x00CF9300
boot_gdt_end:
boot_gdtr:
    boot_gdtr_limit: .word boot_gdt_end-boot_gdt
    boot_gdtr_base:  .long boot_gdt

.comm stack, MULTIBOOT_STACK_SIZE

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
#define ASM
#include "boot.h"
#include "system.h"
#include "task.h"
.global kernel_entry
.global main
.extern check_kernel
.extern init_system_info
.extern setup_kernel
.extern root_task
.extern root_task_entry

#define MULTIBOOT_STACK_SIZE 0x4000

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
    addl    $KRNLADDR,%ebx
    pushl   %ebx

    call boot_paging

    #movl    $init_pgd-KRNLADDR,%ebx
    #movl    %ebx,%cr3

    # enable PG WP
    movl    %cr0,%eax
    orl     $0x80010000,%eax
    movl    %eax,%cr0

    jmp     4f
4:
    # 准备切保护模式
    # 此时esp的地址还需要再调整一下
    addl    $KRNLADDR, %esp
    ljmp    $0x08,$Label+KRNLADDR

Label:
    leal    check_kernel, %eax
    call    *%eax

    movl    $root_task + TASK_SIZE, %esp
    leal    init_system_info, %eax
    call    *%eax;

    leal    setup_kernel, %eax
    call    *%eax

    subl    $128, %esp   // ss esp eip
    leal    root_task_entry, %eax
    jmpl    *%eax

die:
    jmp     die    # Should never come to here.

.align 32
boot_gdt:
EMPT: .long    0x00000000, 0x00000000
Code: .long    0x0000FFFF, 0x00CF9B00
Data: .long    0x0000FFFF, 0x00CF9300
boot_gdt_end:
boot_gdtr:
    boot_gdtr_limit: .word boot_gdt_end-boot_gdt
    boot_gdtr_base:  .long boot_gdt

    .comm stack, MULTIBOOT_STACK_SIZE



.section .multiboot2_header
# An OS image must contain an additional header called Multiboot2 header,
# besides the headers of the format used by the OS image.
# The Multiboot2 header must be contained completely within the first 32768 bytes of the OS image,
# and must be 64-bit aligned.
# In general, it should come as early as possible,
# and may be embedded in the beginning of the text segment after the real executable header.
# 64-bit 对齐 就是 8 字节对齐
.align 8
multiboot2_header_bgn:
    # magic
    .long MULTIBOOT2_HEADER_MAGIC
    # ISA: i386
    .long MULTIBOOT_ARCHITECTURE_I386
    # header length
    .long multiboot2_header_end - multiboot2_header_bgn
    # checksum
    .long   -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (multiboot2_header_end - multiboot2_header_bgn))

#if 1
    /* === Framebuffer Tag === */
    .align 8
    .framebuffer_tag_bgn:
    .short MULTIBOOT_HEADER_TAG_FRAMEBUFFER
    .short MULTIBOOT_HEADER_TAG_OPTIONAL
    .long .framebuffer_tag_end - .framebuffer_tag_bgn

    # width
    # Contains the number of the columns. This is specified in pixels in a graphics mode,
    # and in characters in a text mode.
    # The value zero indicates that the OS image has no preference.
    .long 0

    # height
    # Contains the number of the lines. This is specified in pixels in a graphics mode,
    # and in characters in a text mode.
    # The value zero indicates that the OS image has no preference.
    .long 0

    # depth
    # Contains the number of bits per pixel in a graphics mode, and zero in a text mode.
    # The value zero indicates that the OS image has no preference.
    .long 0

    .framebuffer_tag_end:
#endif
    // // .align 2
    // .multiboot2_header_tag_module:
    // .short MULTIBOOT_HEADER_TAG_MODULE_ALIGN
    // .short 0
    // .long  8
    // .long 0

    .align 8
    .multiboot2_tag_end:
    .short MULTIBOOT_HEADER_TAG_END
    .short 0
    .long  8
multiboot2_header_end:

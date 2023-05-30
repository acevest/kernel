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
.extern init_pgd
.extern init_pgt
.extern kernel_virtual_addr_start
.extern root_task
.extern root_task_entry

#define MULTIBOOT_STACK_SIZE 0x4000

.text
.code32
.align 32
kernel_entry:
main:
    cli

    # Load GDT's Information To GDTR
    lgdt    GDTR-KRNLADDR

    movw    $0x10, %dx
    movw    %dx,%ds
    movw    %dx,%es
    movw    %dx,%ss
    movw    %dx,%fs
    movw    %dx,%gs

    movl    $(stack + MULTIBOOT_STACK_SIZE - KRNLADDR), %esp

    # Reset EFLAGS
    pushl   $0
    popf

    # Save Multiboot Infomation...
    pushl   %eax
    addl    $KRNLADDR,%ebx
    pushl   %ebx

    # Setup Paging
    # Clear Page Directory
    movl    $init_pgd-KRNLADDR,%ebx
    movl    %ebx,%edi
    movl    $1024,%ecx
    xorl    %eax,%eax
    cld
    rep
    stosl


    # Length = BOOT_INIT_PAGETBL_CNT*4M
    # [0x00000000, 0x00000000 + Length)
    # [0xC0000000, 0xC0000000 + Length)
    # 这两个线性地址空间都映射到同一片物理内存空间: [0x00000000, 0x00000000 + Length)


    # 初始化页目录
    # 设置低端线性地址空间的页表项
    movl    %ebx,%edi
    movl    $init_pgt-KRNLADDR,%eax
    addl    $3,%eax
    movl    $BOOT_INIT_PAGETBL_CNT,%ecx
1:
    stosl
    addl    $0x1000,%eax
    loop    1b

    # 设置内核线性地址空间的页表项
    movl    $kernel_virtual_addr_start,%eax
    shrl    $20,%eax
    addl    %ebx,%eax
    movl    %eax,%edi

    movl    $init_pgt-KRNLADDR,%eax
    addl    $3,%eax
    movl    $BOOT_INIT_PAGETBL_CNT,%ecx
2:
    stosl
    addl    $0x1000,%eax
    loop    2b

    # 初始化页表
    # 直接线性映射BOOT_INIT_PAGETBL_CNT*4M物理内存
    movl    $init_pgt-KRNLADDR,%ebx
    movl    %ebx,%edi
    movl    $(BOOT_INIT_PAGETBL_CNT*1024),%ecx
    movl    $3,%eax
    cld
3:
    stosl
    addl    $0x1000,%eax
    loop    3b

    movl    $init_pgd-KRNLADDR,%ebx
    movl    %ebx,%cr3

    # enable PG WP
    movl    %cr0,%eax
    orl     $0x80010000,%eax
    movl    %eax,%cr0

    jmp     4f
4:

    ljmp    $0x08,$Label
Label:
    call    check_kernel
    addl    $8, %esp

    movl    $root_task + TASK_SIZE, %esp
    call    init_system_info

    call    setup_kernel

    subl    $128, %esp   // ss esp eip
    movl    $root_task_entry, %eax
    jmpl    *%eax

Die:
    jmp     Die    # Should never come to here.

.align 32
BootGDT:
EMPT: .long    0x00000000, 0x00000000
Code: .long    0x0000FFFF, 0x00CF9B00
Data: .long    0x0000FFFF, 0x00CF9300
BootGDTEnd:
GDTR:
    GDTRLimit: .word BootGDTEnd-BootGDT
    GDTRBase:  .long BootGDT-KRNLADDR

    .comm stack, MULTIBOOT_STACK_SIZE



.section .multiboot2_header
# section align 64 bits boundary
.align 64
multiboot2_header_bgn:
    # magic
    .long MULTIBOOT2_HEADER_MAGIC
    # ISA: i386
    .long MULTIBOOT_ARCHITECTURE_I386
    # header length
    .long multiboot2_header_end - multiboot2_header_bgn
    # checksum
    .long   -(MULTIBOOT2_HEADER_MAGIC + MULTIBOOT_ARCHITECTURE_I386 + (multiboot2_header_end - multiboot2_header_bgn))

    // .align 8
    // .framebuffer_tag_bgn:
    // .short MULTIBOOT_HEADER_TAG_FRAMEBUFFER
    // .short MULTIBOOT_HEADER_TAG_OPTIONAL
    // .long .framebuffer_tag_end - .framebuffer_tag_bgn
    // .long 800
    // .long 600
    // .long 32
    // .framebuffer_tag_end:

    .align 8
    .multiboot2_tag_end:
    .short MULTIBOOT_HEADER_TAG_END
    .short 0
    .long  8
multiboot2_header_end:

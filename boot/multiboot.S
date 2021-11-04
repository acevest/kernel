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


.text
.code32
.align 32
kernel_entry:
main:
    # Reset EFLAGS
    pushl   $0
    popf

    # Load GDT's Information To GDTR
    lgdt    GDTR-KRNLADDR

    movw    $0x10, %dx
    movw    %dx,%ds
    movw    %dx,%es
    movw    %dx,%ss
    movw    %dx,%fs
    movw    %dx,%gs

    movl    $(stack + MULTIBOOT_STACK_SIZE - KRNLADDR), %esp

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

    # Init Page Directory
    movl    %ebx,%edi
    movl    $init_pgt-KRNLADDR,%eax
    addl    $3,%eax
    movl    $BOOT_INIT_PAGETBL_CNT,%ecx
1:
    stosl
    addl    $0x1000,%eax
    loop    1b

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
    
    # Init Page Table
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



.section .multiboot_header
.align 32
    # Multiboot Header
    # Align 32 bits boundary
    .align 4
    # Magic
    .long MULTIBOOT_HEADER_MAGIC
    # Flags
    .long MULTIBOOT_HEADER_FLAGS
    # Checksum
    .long -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

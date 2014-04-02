/*
 *--------------------------------------------------------------------------
 *   File Name:	reboot.S
 * 
 * Description:	none
 * 
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:	1.0
 * Create Date: Mon Nov 10 15:58:46 2008
 * Last Update: Tue Feb 10 22:39:24 2009
 * 
 *--------------------------------------------------------------------------
 */
#define ASM
#include"boot/multiboot.h"
#include"system.h"
.global	_start
.extern CheckKernel
.extern SetupKernel
.extern	KernelEntry
.extern init_pgd
.extern init_pgt

.section .multiboot_header
.align 32
	# Multiboot Header
	# Align 32 bits boundary
	.align	4
	# Magic
	.long	MULTIBOOT_HEADER_MAGIC
	# Flags
	.long	MULTIBOOT_HEADER_FLAGS
	# Checksum
	.long	-(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_HEADER_FLAGS)

.text
.code32
.align 32
_start:
	# Reset EFLAGS
	pushl	$0
	popf

	# Load GDT's Information To GDTR
	lgdt	GDTR-KRNLADDR

	mov	$0x10, %dx
	mov	%dx,%ds
	mov	%dx,%es
	mov	%dx,%ss
	mov	%dx,%fs
	mov	%dx,%gs

	movl	$(stack + MULTIBOOT_STACK_SIZE - KRNLADDR), %esp


	# Save Multiboot Infomation...
	pushl	%eax
	addl	$KRNLADDR,%ebx
	pushl	%ebx

	# Setup Page 
	movl	$init_pgd-KRNLADDR,%ebx
	movl	%ebx,%edi
	movl	$1024,%ecx
	xorl	%eax,%eax
	cld
	rep
	stosl

	movl	%ebx,%edi
	movl	%ebx,%eax
	movl	%ebx,%ecx
	addl	$7,%eax
	addl	$0x1000,%eax
	movl	%eax, 0000(%edi)
	movl	%eax, 3072(%edi)
	addl	$0x1000,%eax
	movl	%eax, 0004(%edi)
	movl	%eax, 3076(%edi)

	movl	$init_pgt-KRNLADDR,%ebx
	movl	%ebx,%edi
	#addl	$0x1000,%edi
	movl	$2048,%ecx
	movl	$0x07,%eax
	cld
1:
	stosl
	addl	$0x1000,%eax
	loop	1b

	movl	$init_pgd-KRNLADDR,%ebx
	movl	%ebx,%cr3

	# enable PG
	movl	%cr0,%eax
	orl	$0x80000000,%eax
	movl	%eax,%cr0


	jmp	2f
2:

	ljmp	$0x08,$Label
Label:
	call	CheckKernel
	addl	$8,%esp
	call	KernelEntry

Die:	jmp	Die	# Should never come to here.

.align 32
BootGDT:
EMPT:	.long	0x00000000,0x00000000
Code:	.long	0x0000FFFF,0x00CF9B00
Data:	.long	0x0000FFFF,0x00CF9300
BootGDTEnd:
GDTR:
	GDTRLimit:	.word	BootGDTEnd-BootGDT
	GDTRBase:	.long	BootGDT-KRNLADDR

	.comm	stack, MULTIBOOT_STACK_SIZE

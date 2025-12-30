/*
 *--------------------------------------------------------------------------
 *   File Name: link.ld
 *
 * Description: Link All Object Files to KERNEL ...
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version: 1.1
 * Create Date: Mon Mar  2 12:21:03 2009
 * Last Update: Mon Mar  2 12:21:03 2009
 *
 *--------------------------------------------------------------------------
 */

OUTPUT_FORMAT("elf32-i386")
ENTRY(kernel_entry)

/* 内核加载的物理地址 */
kernel_loaded_physic_addr = 0x100000;
/* 内核虚拟地址空间的起始地址 */
kernel_virtual_addr_start = 0xC0000000;
/* 内核起始地址在虚拟地址空间中的起始地址 */
kernel_begin = kernel_virtual_addr_start + kernel_loaded_physic_addr;


SECTIONS
{
    . = kernel_loaded_physic_addr;
    .unpagged_text :
    {
        *(.kernel_entry_text)
        *(.multiboot2_header)
        boot/multiboot.S.o(.text)
        *unpaged_*.o(.text)
    }
    eunpagged_text = .;

    . = ALIGN(0x1000);
    .unpaged_data :
    {
       *unpaged_*.o(.data .rodata*)
    }
    eunpagged_data = .;
    . = ALIGN(0x1000);
    .unpaged_bss : ALIGN(0x1000)
    {
        boot/multiboot.S.o(.bss COMMON)
        *unpaged_*.o(.bss COMMON)
    }
    eunpaged_rodata = .;

    . = ALIGN(0x1000);
    ring3_page_addr = .;
    .ring3 :
    {
        *(.ring3.entry)
    }
    . = ALIGN(0x1000);
    ring3_page_end = .;

    /* . = kernel_begin; */
    . += kernel_virtual_addr_start;
    . = ALIGN(0x1000);
    .text : AT(ADDR(.text)-kernel_virtual_addr_start) ALIGN(0x1000)
    {
        code = .;
        *(.text)
        *(.ring3.text);
        *(.sysexit) /* last */
    }
    etext = .;

    /* 后续节的物理地址(也就是 LMA: Load Memory Address 加载内存地址)会根据 .text 节的自动计算，不用再每个都写一遍 AT 指令了 */

    .data : ALIGN(0x1000)
    {
        data = .;
        *(.data)
    }
    edata = .;


    .bss : ALIGN(0x1000)
    {
        bss = .;
        *(.bss);
    }
    ebss = .;

    end = .;
    _end = .;
    kernel_end = .;

    bootmem_bitmap_begin = ALIGN(0x1000);
}

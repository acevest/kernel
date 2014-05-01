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
kernel_loaded_physic_addr = 0x100000;
kernel_virtual_addr_start = 0xC0000000;
kernel_begin = kernel_virtual_addr_start + kernel_loaded_physic_addr;
SECTIONS
{
    . = kernel_begin;
    .text : AT(phys_addr) ALIGN(0x1000)
    {
        code = .;
        phys_addr = . - kernel_virtual_addr_start;
        *(.multiboot_header)
        *(.text)

        *(.sysexit) /* last */
    }
    etext = .;
    .data : AT(phys_addr) ALIGN(0x1000) 
    {
        data = .;
        phys_addr = . - kernel_virtual_addr_start;
        *(.data)
    }
    edata = .;
    .bss : AT(phys_addr)  ALIGN(0x1000)
    {
        bss = .;
        phys_addr = . - kernel_virtual_addr_start;
        *(.bss);
    }

    ebss = .;

    .init.data : AT(phys_addr) ALIGN(0x1000)
    {
        initdata = .;
        phys_addr = . - kernel_virtual_addr_start;
        *(.init.data);
    }

    end = .;
    _end = .;
    kernel_end = ALIGN(0x1000);
}

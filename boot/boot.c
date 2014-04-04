/*
 *--------------------------------------------------------------------------
 *   File Name: boot.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Wed Dec 30 21:55:29 2009
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */


#include <system.h>
#include <boot/boot.h>
#include <page.h>
#include <bits.h>
#include <assert.h>

extern void    parse_cmdline(const char *cmdline);
void    init_free_area(u32 base, u32 len);


struct boot_params boot_params __attribute__((aligned(32)));

void init_boot_params(multiboot_info_t *p)
{
    boot_params.cmdline = (char *) p->cmdline;

    // KB to Bytes
    // no need to concern about 64bit
    boot_params.mem_lower = p->mem_lower << 10;
    boot_params.mem_upper = p->mem_upper << 10;

    boot_params.boot_device = p->boot_device;

    memory_map_t *mmap = (memory_map_t *) pa2va(p->mmap_addr);

    unsigned int i;
    boot_params.e820map.map_cnt = p->mmap_length / sizeof(memory_map_t);
    for(i=0; i<boot_params.e820map.map_cnt; ++i, ++mmap)
    {
        boot_params.e820map.map[i].addr = mmap->base_addr_low;
        boot_params.e820map.map[i].size = mmap->length_low;
        boot_params.e820map.map[i].type = mmap->type;
    }
}

void CheckKernel(unsigned long addr, unsigned long magic)
{
    if(magic != MULTIBOOT_BOOTLOADER_MAGIC)
    {
        printk("Your boot loader does not support multiboot.\n");
        while(1);
    }

    multiboot_info_t *mbi = (multiboot_info_t *) addr;

    if( (mbi->flags & 0x47) != 0x47)
    {
        printk("Kernel Need More Information\n");
        while(1);
    }

    init_boot_params(mbi);

}

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

#include <assert.h>
#include <bits.h>
#include <boot.h>
#include <page.h>
#include <system.h>

struct boot_params boot_params __attribute__((aligned(32)));

void parse_cmdline(const char *cmdline);

void init_boot_params(multiboot_info_t *p) {
    boot_params.cmdline = (char *)p->cmdline;

    parse_cmdline(boot_params.cmdline);

    // KB to Bytes
    // no need to concern about 64bit
    boot_params.mem_lower = p->mem_lower << 10;
    boot_params.mem_upper = p->mem_upper << 10;

    boot_params.boot_device = p->boot_device;

    multiboot_memory_map_t *mmap = (multiboot_memory_map_t *)pa2va(p->mmap_addr);

    unsigned int i;
    boot_params.e820map.map_cnt = p->mmap_length / sizeof(multiboot_memory_map_t);
    for (i = 0; i < boot_params.e820map.map_cnt; ++i, ++mmap) {
        boot_params.e820map.map[i].addr = mmap->addr;
        boot_params.e820map.map[i].size = mmap->len;
        boot_params.e820map.map[i].type = mmap->type;
    }
}

void check_kernel(unsigned long addr, unsigned long magic) {
    if (magic != MULTIBOOT_BOOTLOADER_MAGIC) {
        printk("Your boot loader does not support multiboot.\n");
        while (1)
            ;
    }

    multiboot_info_t *mbi = (multiboot_info_t *)addr;

    printk("multiboot info flag: %x\n", mbi->flags);

    if (mbi->flags & MULTIBOOT_INFO_BOOT_LOADER_NAME != 0) {
        printk("bootloader: %s\n", (char *)pa2va(mbi->boot_loader_name));
    }

    if (mbi->flags & MULTIBOOT_INFO_VBE_INFO != 0) {
        printk("VBE control info: %x\n", mbi->vbe_control_info);
        printk("VBE mode info: %x\n", mbi->vbe_mode_info);
        printk("VBE mode: %x\n", mbi->vbe_mode);
        printk("VBE interface seg: %x\n", mbi->vbe_interface_seg);
        printk("VBE interface off: %x\n", mbi->vbe_interface_off);
        printk("VBE interface len: %x\n", mbi->vbe_interface_len);
    }

    if (mbi->flags & MULTIBOOT_INFO_FRAMEBUFFER_INFO != 0) {
        printk("frame buffer addr %08x [%dx%d]\n", (unsigned long)(mbi->framebuffer_addr & (~0UL)),
               mbi->framebuffer_width, mbi->framebuffer_height);
        printk("frame buffer pitch %x bpp %x type %x\n", mbi->framebuffer_pitch, mbi->framebuffer_bpp,
               mbi->framebuffer_type);
    }

    if ((mbi->flags & 0x47) != 0x47) {
        printk("KERNEL NEED MORE INFORMATION\n");
        while (1)
            ;
    }

    init_boot_params(mbi);
}

extern void *kernel_begin;
extern void *kernel_end;
extern void *bootmem_bitmap_begin;
void init_system_info() {
    system.kernel_begin = &kernel_begin;
    system.kernel_end = &kernel_end;
    system.bootmem_bitmap_begin = &bootmem_bitmap_begin;

    printk("kernel [%x, %x] bootmem bitmap: %x\n", system.kernel_begin, system.kernel_end, system.bootmem_bitmap_begin);
}

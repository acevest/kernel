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
#include <string.h>
#include <system.h>

struct boot_params boot_params __attribute__((aligned(32)));

void parse_cmdline(const char *cmdline);
void init_vbe(void *, void *);

// ticks < 0 代表永远等待
void boot_delay(int ticks) {
#if ENABLE_BOOT_WAIT
    char chs[] = {'\\', '-', '/', '-'};
    uint32_t cnt = 0;

    printk(" ");
    asm("sti;");
    while (true) {
        if (ticks == 0) {
            break;
        }

        if (ticks > 0) {
            ticks--;
        }

        printk("\b%c", chs[(cnt++ / 3) % sizeof(chs)]);
        asm("hlt");
    }
    asm("cli;");
    printk("\b \b");
#endif
}

void init_ttys();
void setup_gdt();
void setup_idt();
void setup_gates();
void set_tss();
void setup_i8253(uint16_t);
void setup_boot_irqs();

void check_kernel(unsigned long addr, unsigned long magic) {
    init_ttys();

    printk("setup gdt\n");
    setup_gdt();

    printk("setup idt\n");
    setup_idt();

    printk("setup trap and interrupt gates\n");
    setup_gates();

    // 在初始化阶段一直运行在特权级0上
    // 在正在进入用户态前,所有中断都不会涉及特权级变化
    // 自然就不会有栈切换
    // 因此这里set_tss里的tss.esp0是不用初始化的
    set_tss();

    setup_boot_irqs();

    setup_i8253(100);

    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        printk("Your boot loader does not support multiboot.\n");
        boot_delay(-1);
    }
    unsigned long total_size = *((unsigned long *)addr);
    struct multiboot_tag *tag = (struct multiboot_tag *)(addr + 8);  // 跳过中间的 reserved 字段

    printk("total size: %d tags: %x\n", total_size, tag);
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);
    struct multiboot_tag_basic_meminfo *mminfo = 0;
    struct multiboot_tag_bootdev *bootdev = 0;
    struct multiboot_tag_mmap *mmap_tag = 0;
    struct multiboot_tag_vbe *vbe = 0;
    struct multiboot_tag_framebuffer *fb = 0;

    boot_params.e820map.map_cnt = 0;

    while (tag->type != MULTIBOOT_TAG_TYPE_END) {
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_CMDLINE:
            strlcpy(boot_params.cmdline, ((struct multiboot_tag_string *)tag)->string, sizeof(boot_params.cmdline));
            parse_cmdline(boot_params.cmdline);
            break;
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
            strlcpy(boot_params.bootloader, ((struct multiboot_tag_string *)tag)->string,
                    sizeof(boot_params.bootloader));
            break;
        case MULTIBOOT_TAG_TYPE_BASIC_MEMINFO:
            mminfo = (struct multiboot_tag_basic_meminfo *)tag;
            // KB to Bytes
            // no need to concern about 64bit
            boot_params.mem_lower = mminfo->mem_lower << 10;
            boot_params.mem_upper = mminfo->mem_upper << 10;
            break;
        case MULTIBOOT_TAG_TYPE_BOOTDEV:
            bootdev = (struct multiboot_tag_bootdev *)tag;
            boot_params.biosdev = bootdev->biosdev;
            boot_params.partition = bootdev->slice;
            boot_params.sub_partition = bootdev->part;
            break;
        case MULTIBOOT_TAG_TYPE_MMAP:
            mmap_tag = (struct multiboot_tag_mmap *)tag;
            multiboot_memory_map_t *mmap = mmap_tag->entries;
            while (((multiboot_uint32_t)mmap) < (((multiboot_uint32_t)mmap_tag) + mmap_tag->size)) {
                boot_params.e820map.map[boot_params.e820map.map_cnt].addr = mmap->addr;
                boot_params.e820map.map[boot_params.e820map.map_cnt].size = mmap->len;
                boot_params.e820map.map[boot_params.e820map.map_cnt].type = mmap->type;
                boot_params.e820map.map_cnt++;
                mmap = (multiboot_memory_map_t *)(((unsigned long)mmap) + mmap_tag->entry_size);
            }
            break;
        case MULTIBOOT_TAG_TYPE_VBE:
            vbe = (struct multiboot_tag_vbe *)tag;
            void *vci = (void *)vbe->vbe_control_info.external_specification;
            void *vmi = (void *)vbe->vbe_mode_info.external_specification;
            // vbe->vbe_control_info;
            // asm volatile("xchg %%bx, %%bx;nop;nop;" ::"a"(vci), "b"(vmi));
            // asm volatile("xchg %%bx, %%bx;nop;nop;" ::"a"(vbe->vbe_interface_seg), "b"(vbe->vbe_interface_off),
            //              "c"(vbe->vbe_interface_len));
            init_vbe(vci, vmi);
            break;
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
            // asm volatile("xchg %bx, %bx;nop;nop;nop;nop;");
            break;
        default:
            printk("tag %x size %x\n", tag->type, tag->size);
            break;
        }
        // next tag
        unsigned long size = (tag->size + 7) & (~7UL);
        tag = (struct multiboot_tag *)(((unsigned long)tag) + size);
    }

    boot_delay(DEFAULT_BOOT_DELAY_TICKS);
#if 0
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
    while (1)
        ;

    if ((mbi->flags & 0x47) != 0x47) {
        printk("KERNEL NEED MORE INFORMATION\n");
        while (1)
            ;
    }

    init_boot_params(mbi);

    boot_delay(DEFAULT_BOOT_DELAY_TICKS);
#endif
}

extern void *kernel_begin;
extern void *kernel_end;
extern void *bootmem_bitmap_begin;

void init_system_info() {
    system.kernel_begin = &kernel_begin;
    system.kernel_end = &kernel_end;
    system.bootmem_bitmap_begin = &bootmem_bitmap_begin;

    printk("kernel [%x, %x] bootmem bitmap: %x\n", system.kernel_begin, system.kernel_end, system.bootmem_bitmap_begin);

    printk("bootloader: %s\n", boot_params.bootloader);
    printk("boot device: bios dev %x partition %x sub partition %x\n", boot_params.biosdev, boot_params.partition,
           boot_params.sub_partition);
    printk("mem lower %uKB upper %uKB\n", boot_params.mem_lower >> 10, boot_params.mem_upper >> 10);

    boot_delay(DEFAULT_BOOT_DELAY_TICKS);
}

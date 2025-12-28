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
#include <msr.h>


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

void init_serial();
void init_ttys();
void setup_gdt();
void setup_idt();
void setup_gates();
void set_tss();
void setup_i8254(uint16_t);
void setup_boot_irqs();

void check_kernel(unsigned long addr, unsigned long magic) {
    init_serial();

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

    setup_i8254(100);

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
        bool support = true;
        switch (tag->type) {
        case MULTIBOOT_TAG_TYPE_CMDLINE:
            strlcpy(boot_params.cmdline, ((struct multiboot_tag_string *)tag)->string, sizeof(boot_params.cmdline));
            parse_cmdline(boot_params.cmdline);
            break;
        case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
            strlcpy(boot_params.bootloader, ((struct multiboot_tag_string *)tag)->string,
                    sizeof(boot_params.bootloader));
            break;
        case MULTIBOOT_TAG_TYPE_MODULE:
            struct multiboot_tag_module *m = (struct multiboot_tag_module *)tag;
            void *mod_start = (void *)m->mod_start;
            printk("module 0x%08x - 0x%08x size %u cmdline %s\n", m->mod_start, m->mod_end, m->size, m->cmdline);
            boot_params.boot_module_begin = (void *)m->mod_start;
            boot_params.boot_module_end = (void *)m->mod_end;
#if 1
            const uint32_t mod_magic = *(uint32_t *)(mod_start + 0);
            const uint32_t mod_head_size = *(uint32_t *)(mod_start + 4);
            const uint32_t mod_timestamp = *(uint32_t *)(mod_start + 8);
            const uint32_t mod_file_entry_cnt = *(uint32_t *)(mod_start + 12);
            const char *mod_name = (const char *)mod_start + 16;
            printk("module magic %08x header size %u timestamp %u file entry cnt %u name %s \n", mod_magic,
                   mod_head_size, mod_timestamp, mod_file_entry_cnt, mod_name);
            void timestamp_to_date(uint32_t ts);
            timestamp_to_date(mod_timestamp);

            int file_entry_offset = mod_head_size;
            for (int i = 0; i < mod_file_entry_cnt; i++) {
                void *fe = mod_start + file_entry_offset;

                const uint32_t fe_size = *(uint32_t *)(fe + 0);
                const uint32_t fe_type = *(uint32_t *)(fe + 4);
                const uint32_t fe_filesz = *(uint32_t *)(fe + 8);
                const uint32_t fe_offset = *(uint32_t *)(fe + 12);
                const char *fe_name = (const char *)(fe + 16);

                file_entry_offset += fe_size;

                void *fc = mod_start + fe_offset;

                printk("[fe:%u:%u] file size %u type %u name %s\n", i, fe_size, fe_filesz, fe_type, fe_name);

                for (int k = 0; k < 16; k++) {
                    uint8_t c = *(uint8_t *)(fc + k);
                    printk("%02X ", c);
                }
                printk("\n");
            }
#endif
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
            printk("VBE MODE %04x\n", vbe->vbe_mode);
            init_vbe(vci, vmi);
            break;
        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
            printk("frame buffer\n");
            break;
        // case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
        //     struct multiboot_tag_elf_sections *s = (struct multiboot_tag_elf_sections *)tag;
        //     break;
        case MULTIBOOT_TAG_TYPE_LOAD_BASE_ADDR:
            printk("load base addr %08x\n", ((struct multiboot_tag_load_base_addr *)tag)->load_base_addr);
        default:
            support = false;
            break;
        }
        printk("tag %x size %x\t[%ssupport]\n", tag->type, tag->size, support ? "" : "un");
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

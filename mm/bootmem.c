/*
 * ------------------------------------------------------------------------
 *   File Name: bootmem.c
 *      Author: Zhao Yanbai
 *              2021-11-02 09:07:32 Tuesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */
#include <boot.h>
#include <mm.h>
#include <printk.h>
#include <string.h>
#include <system.h>

static void e820_print_type(unsigned long type) {
    switch (type) {
    case E820_RAM:
        printk("usable");
        break;
    case E820_RESERVED:
        printk("reserved");
        break;
    case E820_ACPI:
        printk("ACPI data");
        break;
    case E820_NVS:
        printk("ACPI NVS");
        break;
    case E820_UNUSABLE:
        printk("unusable");
        break;
    default:
        printk("type %x", type);
        break;
    }
}

// op: 0 clear bit, 1 set bit
void fast_init_bootmem_bitmap(unsigned long bgn_pfn, unsigned long end_pfn, int op) {
    int (*bit_func)(unsigned int, volatile unsigned long *);

    bit_func = op == 0 ? test_and_clear_bit : test_and_set_bit;

    u8 data = op == 0 ? 0x00 : 0xFF;

    // 先设置头部不是从单个字节开始的比特
    unsigned int i = 0;  // 这个变量不能放到for循环里定义
    for (i = bgn_pfn; i < end_pfn && (i % 8 != 0); i++) {
        bit_func(i, bootmem_data.bitmap);
    }

    // 算出中间的整字节数
    unsigned int bytes = (end_pfn - i) / 8;

    // 直接清零或设置
    memset((char *)(bootmem_data.bitmap) + (i / 8), data, bytes);

    // 最后设置尾部不是整字节的比特
    for (i += bytes * 8; i < end_pfn; i++) {
        bit_func(i, bootmem_data.bitmap);
    }
}

void e820_print_map() {
    unsigned int i = 0;

    for (i = 0; i < boot_params.e820map.map_cnt; ++i) {
        struct e820_entry *p = boot_params.e820map.map + i;

        printk(" [%02d] 0x%08x - 0x%08x size %- 10d %8dKB %5dMB ", i, p->addr, p->addr + p->size - 1, p->size,
               p->size >> 10, p->size >> 20);

        e820_print_type(p->type);

        printk("\n");
    }
}

bootmem_data_t bootmem_data;

unsigned long bootmem_max_pfn() { return bootmem_data.max_pfn; }

unsigned long bootmem_page_state(unsigned long pfn) { return constant_test_bit(pfn, bootmem_data.bitmap); }

void e820_init_bootmem_data() {
    unsigned int i = 0;

    memset(&bootmem_data, 0, sizeof(bootmem_data));
    bootmem_data.min_pfn = ~0UL;
    bootmem_data.max_pfn = 0;

    for (i = 0; i < boot_params.e820map.map_cnt; ++i) {
        struct e820_entry *p = boot_params.e820map.map + i;

        if (p->type != E820_RAM) {
            continue;
        }

        unsigned long bgn_pfn = PFN_UP(p->addr);
        unsigned long end_pfn = PFN_DW(p->addr + p->size);

        if (bootmem_data.min_pfn > bgn_pfn) {
            bootmem_data.min_pfn = bgn_pfn;
        }

        if (bootmem_data.max_pfn < end_pfn) {
            bootmem_data.max_pfn = end_pfn;
        }
    }

    // limit max_pfn
    unsigned long max_support_pfn = PFN_DW(MAX_SUPT_PHYMM_SIZE);
    if (bootmem_data.max_pfn > max_support_pfn) {
        bootmem_data.max_pfn = max_support_pfn;
        unsigned long ms = max_support_pfn / 1024 * 4;  // 1024个pfn正好4MB
        printk("memory > %dMB, only support to %dMB.\n", ms, ms);
    }

    printk("pfn_min: %d pfn_max: %d\n", bootmem_data.min_pfn, bootmem_data.max_pfn);

    bootmem_data.prepare_alloc_pfn = bootmem_data.min_pfn;
}

void register_bootmem_pages() {
    for (unsigned int i = 0; i < boot_params.e820map.map_cnt; ++i) {
        struct e820_entry *p = boot_params.e820map.map + i;

        if (p->type != E820_RAM) {
            continue;
        }

        unsigned long bgn_pfn = PFN_UP(p->addr);
        unsigned long end_pfn = PFN_DW(p->addr + p->size);

#if 1
        // 用一个相对快的方式
        fast_init_bootmem_bitmap(bgn_pfn, end_pfn, 0);
#else
        for (unsigned int j = bgn_pfn; j < end_pfn; j++) {
            test_and_clear_bit(j, bootmem_data.bitmap);
        }
#endif
    }
}

void reserve_bootmem(unsigned long bgn_pfn, unsigned long end_pfn) {
#if 1
    // 用一个相对快的方式
    fast_init_bootmem_bitmap(bgn_pfn, end_pfn, 1);
#else
    int i = 0;
    for (i = bgn_pfn; i < end_pfn; ++i) {
        test_and_set_bit(i, bootmem_data.bitmap);
    }
#endif
}

void reserve_kernel_pages() { reserve_bootmem(PFN_DW(va2pa(system.kernel_begin)), PFN_UP(va2pa(system.kernel_end))); }

void reserve_bootmem_bitmap() {
    unsigned long bgn_pfn = PFN_DW(va2pa(bootmem_data.bitmap));

    unsigned long end_pfn = bgn_pfn + PFN_UP(bootmem_data.mapsize);

    reserve_bootmem(bgn_pfn, end_pfn);
}

void init_bootmem_allocator() {
    int mapsize = (bootmem_data.max_pfn + 7) / 8;

    bootmem_data.bitmap = system.bootmem_bitmap_begin;
    bootmem_data.mapsize = mapsize;

    memset(bootmem_data.bitmap, 0xFF, mapsize);

    // 根据bootmem信息初始化空闲内存块
    register_bootmem_pages();

    // 保留内核内存段
    reserve_kernel_pages();

    // 保留管理所有空闲内存的bitmap所占用的内存
    reserve_bootmem_bitmap();

    // 强制保留最开始的一页
    // 免得alloc的时候分不清是失败，还是分配的第0页
    reserve_bootmem(0, 1);
}

void init_bootmem() {
    e820_print_map();
    e820_init_bootmem_data();
    init_bootmem_allocator();
}

// 由于只有在构建buddy system的时候才会用到
// 所以这里就简单实现
void *alloc_from_bootmem(unsigned long size, char *title) {
    void *region = NULL;
    unsigned long pfn_cnt = PFN_UP(size);

    bootmem_data_t *pbd = &bootmem_data;

    // 从该处开始查找空闲区间
    unsigned long search_bgn_pfn = pbd->prepare_alloc_pfn;

find_next_block:
    // 先找到第一个空闲的pfn
    unsigned long free_pfn = pbd->max_pfn;
    for (unsigned long pfn = search_bgn_pfn; pfn < pbd->max_pfn; pfn++) {
        if (bootmem_page_state(pfn) == BOOTMEM_PAGE_FREE) {
            free_pfn = pfn;
            break;
        }
        // printk("pfn %d alloced\n", pfn);
    }

    // 检验接下来是否有足够的size
    unsigned long bgn_pfn = free_pfn;
    unsigned long end_pfn = bgn_pfn + pfn_cnt;

    // printk("free_pfn: %d end_pfn:%d max_pfn:%d \n", free_pfn, end_pfn, pbd->max_pfn);

    // 剩余空间不足
    if (bgn_pfn >= pbd->max_pfn || end_pfn >= pbd->max_pfn) {
        return region;
    }

    //
    for (unsigned long pfn = bgn_pfn; pfn < end_pfn; pfn++) {
        if (bootmem_page_state(pfn) != BOOTMEM_PAGE_FREE) {
            search_bgn_pfn = pfn + 1;
            goto find_next_block;
        }
    }

    reserve_bootmem(bgn_pfn, end_pfn);
    region = pfn2va(bgn_pfn);

    pbd->prepare_alloc_pfn = end_pfn;

    printk("%s alloc bootmem size: %x pfn cnt: %d [%d, %d)\n", title, size, pfn_cnt, bgn_pfn, end_pfn);

    return region;
}

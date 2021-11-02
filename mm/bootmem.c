/*
 * ------------------------------------------------------------------------
 *   File Name: bootmem.c
 *      Author: Zhao Yanbai
 *              2021-11-02 09:07:32 Tuesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */
#include <boot/boot.h>
#include <mm.h>
#include <printk.h>
#include <system.h>

extern char kernel_begin, kernel_end;

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

void e820_print_map() {
    unsigned int i = 0;

    for (i = 0; i < boot_params.e820map.map_cnt; ++i) {
        struct e820_entry *p = boot_params.e820map.map + i;

        printk(" [%02d] 0x%08x - 0x%08x size %- 10d %8dKB %5dMB ", i, p->addr, p->addr + p->size - 1, p->size, p->size >> 10, p->size >> 20);

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

    unsigned long bgn_pfn;
    unsigned long end_pfn;

    for (i = 0; i < boot_params.e820map.map_cnt; ++i) {
        struct e820_entry *p = boot_params.e820map.map + i;

        if (p->type != E820_RAM) continue;

        bgn_pfn = PFN_UP(p->addr);
        end_pfn = PFN_DW(p->addr + p->size);

        if (bootmem_data.max_pfn < end_pfn) bootmem_data.max_pfn = end_pfn;
    }

    bootmem_data.min_pfn = 0;

    // limit max_pfn
    unsigned long max_support_pfn = PFN_DW(MAX_SUPT_PHYMM_SIZE);
    if (bootmem_data.max_pfn > max_support_pfn) {
        bootmem_data.max_pfn = max_support_pfn;
    }
}

void register_bootmem_pages() {
    unsigned int i = 0;
    unsigned int j = 0;

    for (i = 0; i < boot_params.e820map.map_cnt; ++i) {
        struct e820_entry *p = boot_params.e820map.map + i;

        if (p->type != E820_RAM) continue;

        unsigned long bgn_pfn = PFN_UP(p->addr);
        unsigned long end_pfn = PFN_DW(p->addr + p->size);

        for (j = bgn_pfn; j < end_pfn; ++j) {
            test_and_clear_bit(j, bootmem_data.bitmap);
        }
    }
}

void reserve_bootmem(unsigned long bgn_pfn, unsigned long end_pfn) {
    // printk("reserve %d %d\n", bgn_pfn, end_pfn);

    int i = 0;
    for (i = bgn_pfn; i < end_pfn; ++i) {
        test_and_set_bit(i, bootmem_data.bitmap);
    }
}

void reserve_kernel_pages() {
    // reserve_bootmem(PFN_DW(va2pa(&kernel_begin)), PFN_UP(va2pa(&kernel_end)));
    reserve_bootmem(0, PFN_UP(va2pa(&kernel_end)));
}

void reserve_bootmem_pages() {
    unsigned long bgn_pfn = PFN_DW(va2pa(bootmem_data.bitmap));

    unsigned long end_pfn = bgn_pfn + PFN_UP(bootmem_data.mapsize);

    reserve_bootmem(bgn_pfn, end_pfn);
}

void init_bootmem_allocator() {
    int mapsize = (bootmem_data.max_pfn + 7) / 8;

    bootmem_data.bitmap = &kernel_end;
    bootmem_data.mapsize = mapsize;

    memset(bootmem_data.bitmap, 0xFF, mapsize);

    register_bootmem_pages();

    reserve_kernel_pages();

    reserve_bootmem_pages();
}

void init_bootmem() {
    e820_print_map();
    e820_init_bootmem_data();
    init_bootmem_allocator();

#if 0
    printk("alloc 10 bytes align 8    addr %08x\n", alloc_bootmem(10, 8));
    printk("alloc 40961 bytes align 4096 addr %08x\n", alloc_bootmem(40961, 4096));
    printk("alloc 5  bytes align 4    addr %08x\n", alloc_bootmem(5, 4));
    printk("alloc 10 bytes align 1024 addr %08x\n", alloc_bootmem(10, 1024));
    printk("alloc 123bytes align 2    addr %08x\n", alloc_bootmem(123, 2));
    printk("alloc 123bytes align 2    addr %08x\n", alloc_bootmem(123, 2));
#endif
}

void *alloc_bootmem(unsigned long size, unsigned long align) {
    bootmem_data_t *pbd = &bootmem_data;

    assert(size != 0);
    assert((align & (align - 1)) == 0);  // must be power of 2

    unsigned long fallback = 0;
    unsigned long bgn_pfn, end_pfn, step;

    step = align >> PAGE_SHIFT;
    step = step > 0 ? step : 1;

    bgn_pfn = ALIGN(pbd->min_pfn, step);
    end_pfn = pbd->max_pfn;

    // start from last position
    if (pbd->last_hit_pfn > bgn_pfn) {
        fallback = bgn_pfn + 1;
        bgn_pfn = ALIGN(pbd->last_hit_pfn, step);
    }

    while (1) {
        int merge;
        void *region;
        unsigned long i, search_end_pfn;
        unsigned long start_off, end_off;

    find_block:

        bgn_pfn = find_next_zero_bit(pbd->bitmap, end_pfn, bgn_pfn);
        bgn_pfn = ALIGN(bgn_pfn, step);

        search_end_pfn = bgn_pfn + PFN_UP(size);

        if (bgn_pfn >= end_pfn || search_end_pfn > end_pfn) break;

        for (i = bgn_pfn; i < search_end_pfn; ++i) {
            if (bootmem_page_state(i) != BOOTMEM_PAGE_FREE) {  // space not enough
                bgn_pfn = ALIGN(i, step);
                if (bgn_pfn == i) bgn_pfn += step;

                goto find_block;
            }
        }

        // try to use the unused part of last page
        if (pbd->last_offset & (PAGE_SIZE - 1) && PFN_DW(pbd->last_offset) + 1 == bgn_pfn)
            start_off = ALIGN(pbd->last_offset, align);
        else
            start_off = pfn2pa(bgn_pfn);

        merge = PFN_DW(start_off) < bgn_pfn;
        end_off = start_off + size;

        pbd->last_offset = end_off;
        pbd->last_hit_pfn = PFN_UP(end_off);

        reserve_bootmem(PFN_DW(start_off) + merge, PFN_UP(end_off));

        region = pa2va(start_off);

        memset(region, 0, size);

        return region;
    }

    if (fallback) {
        bgn_pfn = ALIGN(fallback - 1, step);
        fallback = 0;
        goto find_block;
    }

    return 0;
}
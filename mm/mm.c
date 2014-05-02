/*
 *--------------------------------------------------------------------------
 *   File Name: mm.c
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Wed Mar  4 21:08:47 2009
 * Last Update: Wed Mar  4 21:08:47 2009
 * 
 *--------------------------------------------------------------------------
 */
#include <printk.h>
#include <system.h>
#include <page.h>
#include <types.h>
#include <bits.h>
#include <mm.h>
#include <init.h>
#include <boot/boot.h>


extern char kernel_begin, kernel_end;
extern char etext,edata,end;
extern void init_buddy_system();
extern void init_slub_system();

static void e820_print_type(unsigned long type)
{
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


void e820_print_map()
{
    unsigned int i=0;

    for(i=0; i<boot_params.e820map.map_cnt; ++i)
    {
        struct e820_entry *p = boot_params.e820map.map + i;

        printk("[%02d] 0x%08x - 0x%08x size %- 10d %8dKB %5dMB ", i, p->addr, p->addr + p->size, p->size, p->size>>10, p->size>>20);

        e820_print_type(p->type);

        printk("\n");
    }
}

typedef struct bootmem_data {
    unsigned long min_pfn;
    unsigned long max_pfn;

    unsigned long last_offset;   // offset to pfn2pa(this->min_pfn);
    unsigned long last_hit_pfn; // last hit index in bitmap

    void *bitmap;   
    unsigned long mapsize;
} bootmem_data_t;


bootmem_data_t bootmem_data;

unsigned long bootmem_total_pages()
{
    return bootmem_data.max_pfn;
}

unsigned long bootmem_page_state(unsigned long pfn)
{
    return constant_test_bit(pfn, bootmem_data.bitmap);
}

void e820_init_bootmem_data()
{
    unsigned int i=0;

    memset(&bootmem_data, 0, sizeof(bootmem_data));
    bootmem_data.min_pfn    = ~0UL;

    unsigned long bgn_pfn;
    unsigned long end_pfn;

    for(i=0; i<boot_params.e820map.map_cnt; ++i)
    {
        struct e820_entry *p = boot_params.e820map.map + i;

        if(p->type != E820_RAM)
            continue;

        bgn_pfn = PFN_UP(p->addr);
        end_pfn = PFN_DW(p->addr + p->size);

        if(bootmem_data.max_pfn < end_pfn)
            bootmem_data.max_pfn = end_pfn;
    }

    bootmem_data.min_pfn = 0;

    // limit max_pfn
    unsigned long max_support_pfn = PFN_DW(MAX_SUPT_PHYMM_SIZE);
    if(bootmem_data.max_pfn > max_support_pfn)
    {
        bootmem_data.max_pfn = max_support_pfn;
    }
}

void register_bootmem_pages()
{
    unsigned int i=0;
    unsigned int j=0;

    for(i=0; i<boot_params.e820map.map_cnt; ++i)
    {
        struct e820_entry *p = boot_params.e820map.map + i;

        if(p->type != E820_RAM)
            continue;

        unsigned long bgn_pfn = PFN_UP(p->addr);
        unsigned long end_pfn = PFN_DW(p->addr + p->size);

        for(j=bgn_pfn; j<end_pfn; ++j)
        {
            test_and_clear_bit(j, bootmem_data.bitmap);
        }
    } 
}

void reserve_bootmem(unsigned long bgn_pfn, unsigned long end_pfn)
{
    //printk("reserve %d %d\n", bgn_pfn, end_pfn);

    int i=0;
    for(i=bgn_pfn; i<end_pfn; ++i)
    {
        test_and_set_bit(i, bootmem_data.bitmap);
    }
}

void reserve_kernel_pages()
{
    //reserve_bootmem(PFN_DW(va2pa(&kernel_begin)), PFN_UP(va2pa(&kernel_end)));
    reserve_bootmem(0, PFN_UP(va2pa(&kernel_end)));
}

void reserve_bootmem_pages()
{
    unsigned long bgn_pfn = PFN_DW(va2pa(bootmem_data.bitmap));

    unsigned long end_pfn = bgn_pfn + PFN_UP(bootmem_data.mapsize);

    reserve_bootmem(bgn_pfn, end_pfn);
}

void init_bootmem_allocator()
{
    int mapsize = (bootmem_data.max_pfn + 7) / 8;

    bootmem_data.bitmap = &kernel_end;
    bootmem_data.mapsize= mapsize;

    memset(bootmem_data.bitmap, 0xFF, mapsize);

    register_bootmem_pages();

    reserve_kernel_pages();

    reserve_bootmem_pages();
}

void init_bootmem()
{
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

void *alloc_bootmem(unsigned long size, unsigned long align)
{
    bootmem_data_t *pbd = &bootmem_data;

    assert(size != 0);
    assert((align & (align-1)) == 0); // must be power of 2
    
    unsigned long fallback = 0;
    unsigned long bgn_pfn, end_pfn, step;

    step = align >> PAGE_SHIFT;
    step = step > 0 ? step : 1;

    bgn_pfn = ALIGN(pbd->min_pfn, step);
    end_pfn = pbd->max_pfn;

    // start from last position
    if(pbd->last_hit_pfn > bgn_pfn)
    {
        fallback = bgn_pfn + 1;
        bgn_pfn  = ALIGN(pbd->last_hit_pfn, step);
    }

    while(1)
    {
        int merge;
        void *region;
        unsigned long i, search_end_pfn;
        unsigned long start_off, end_off;

find_block:

        bgn_pfn = find_next_zero_bit(pbd->bitmap, end_pfn, bgn_pfn);
        bgn_pfn = ALIGN(bgn_pfn, step);

        search_end_pfn = bgn_pfn + PFN_UP(size);

        if(bgn_pfn >= end_pfn || search_end_pfn > end_pfn)
            break;

        for(i=bgn_pfn; i<search_end_pfn; ++i)
        {
            if(bootmem_page_state(i) != BOOTMEM_PAGE_FREE) {    // space not enough
                bgn_pfn = ALIGN(i, step);
                if(bgn_pfn == i)
                    bgn_pfn += step;

                goto find_block;
            }
        }

        // try to use the unused part of last page
        if(pbd->last_offset & (PAGE_SIZE - 1) && PFN_DW(pbd->last_offset) + 1 == bgn_pfn)
            start_off = ALIGN(pbd->last_offset, align);
        else
            start_off = pfn2pa(bgn_pfn);

        merge   = PFN_DW(start_off) < bgn_pfn;
        end_off = start_off + size;

        pbd->last_offset  = end_off;
        pbd->last_hit_pfn = PFN_UP(end_off);

        reserve_bootmem(PFN_DW(start_off) + merge, PFN_UP(end_off));
        
        region = pa2va(start_off);

        memset(region, 0, size);

        return region;
    }

    if(fallback)
    {
        bgn_pfn = ALIGN(fallback-1, step);
        fallback = 0;
        goto find_block;
    }

    return 0;
}


pde_t __initdata init_pgd[PDECNT_PER_PAGE]                       __attribute__((__aligned__(PAGE_SIZE)));
pte_t __initdata init_pgt[PTECNT_PER_PAGE*BOOT_INIT_PAGETBL_CNT] __attribute__((__aligned__(PAGE_SIZE)));

void set_page_shared(void *x)
{
    unsigned long addr = (unsigned long) x;
    addr = PAGE_ALIGN(addr);
    unsigned int npd = get_npd(addr);
    init_pgd[npd] |= PAGE_US;

    pte_t *pte = pa2va(init_pgd[npd] & PAGE_MASK);
    pte[get_npt(addr)] |= PAGE_US;
}

extern void sysexit();

void init_paging()
{
    unsigned int i;
    unsigned long pfn = 0;
    pte_t *pte = 0;
    unsigned long pgtb_addr = 0;
    for(pfn=pa2pfn(BOOT_INIT_PAGETBL_CNT<<22); pfn<bootmem_data.max_pfn; ++pfn)
    {
        unsigned long ti = pfn % PAGE_PTE_CNT;
        unsigned long page_addr = pfn2pa(pfn);
        if(ti == 0)
        {
            pgtb_addr = (unsigned long) va2pa(bootmem_alloc_pages(1));
            if(0 == pgtb_addr)
                panic("No Pages for Paging...");

            memset((void *)pgtb_addr, 0, PAGE_SIZE);

            init_pgd[get_npd(page_addr)] = (pde_t)(pgtb_addr | PAGE_P | PAGE_WR | PAGE_US);
        }

        pte = ((pte_t *) pa2va(pgtb_addr)) + ti;
        *pte = (pte_t) (page_addr | PAGE_P | PAGE_WR | PAGE_US);
    }


    // paging for kernel space
    unsigned long delta = get_npd(PAGE_OFFSET);
    for(i=delta; i<PDECNT_PER_PAGE; ++i)
    {
        init_pgd[i] = init_pgd[i-delta];
        init_pgd[i-delta] = 0;
    }

    // paging for user space
    // set_page_shared(sysexit);

    LOAD_CR3(init_pgd);
}

void init_mm()
{
    printk("init bootmem alloc...\n");
    init_bootmem();
    printk("init global paging...\n");
    init_paging();
    printk("init buddy system...\n");
    init_buddy_system();
    printk("init slub system...\n");
    init_slub_system();
    printk("memory init finished...\n");
}

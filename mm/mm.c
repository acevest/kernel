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
#include <bits.h>
#include <boot.h>
#include <linkage.h>
#include <mm.h>
#include <page.h>
#include <printk.h>
#include <system.h>
#include <types.h>

extern char etext, edata, end;
extern void init_buddy_system();
extern void init_slub_system();

pde_t __initdata init_pgd[PDECNT_PER_PAGE] __attribute__((__aligned__(PAGE_SIZE)));
pte_t __initdata init_pgt[PTECNT_PER_PAGE * BOOT_INIT_PAGETBL_CNT] __attribute__((__aligned__(PAGE_SIZE)));

void set_page_shared(void *x) {
    unsigned long addr = (unsigned long)x;
    addr = PAGE_ALIGN(addr);
    unsigned int npd = get_npd(addr);
    init_pgd[npd] |= PAGE_US;

    pte_t *pte = pa2va(init_pgd[npd] & PAGE_MASK);
    pte[get_npt(addr)] |= PAGE_US;
}

void init_paging() {
    unsigned int i;
    unsigned long pfn = 0;
    pte_t *pte = 0;
    unsigned long pgtb_addr = 0;

    // 在multiboot.S是已经初始化了BOOT_INIT_PAGETBL_CNT个页
    // 这里接着初始化剩余的页
    // 最大限制内存1G
    for (pfn = pa2pfn(BOOT_INIT_PAGETBL_CNT << 22); pfn < bootmem_data.max_pfn; ++pfn) {
        unsigned long ti = pfn % PAGE_PTE_CNT;
        unsigned long page_addr = pfn2pa(pfn);
        if (ti == 0) {
            pgtb_addr = (unsigned long)va2pa(alloc_from_bootmem(PAGE_SIZE, "paging"));
            if (0 == pgtb_addr) panic("No Pages for Paging...");

            memset((void *)pgtb_addr, 0, PAGE_SIZE);

            init_pgd[get_npd(page_addr)] = (pde_t)(pgtb_addr | PAGE_P | PAGE_WR);
        }

        pte = ((pte_t *)pa2va(pgtb_addr)) + ti;
        *pte = (pte_t)(page_addr | PAGE_P | PAGE_WR);
    }

    // paging for kernel space
    unsigned long delta = get_npd(PAGE_OFFSET);
    for (i = delta; i < PDECNT_PER_PAGE; ++i) {
        init_pgd[i] = init_pgd[i - delta];
        init_pgd[i - delta] = 0;
    }

    // paging for user space
    extern void sysenter();
    extern void sysexit();
    set_page_shared(sysenter);
    set_page_shared(sysexit);

    LoadCR3(va2pa(init_pgd));
}

void init_mm() {
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

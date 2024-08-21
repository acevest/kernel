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
#include <string.h>
#include <system.h>
#include <types.h>

extern char etext, edata, end;

pde_t __initdata init_pgd[PDECNT_PER_PAGE] __attribute__((__aligned__(PAGE_SIZE)));
pte_t __initdata init_pgt[PTECNT_PER_PAGE * BOOT_INIT_PAGETBL_CNT] __attribute__((__aligned__(PAGE_SIZE)));

void set_page_shared(void *x) {
    unsigned long addr = (unsigned long)x;
    addr = PAGE_ALIGN(addr);
    unsigned int npd = get_npde(addr);
    init_pgd[npd] |= PAGE_US;

    pte_t *pte = pa2va(init_pgd[npd] & PAGE_MASK);
    pte[get_npte(addr)] |= PAGE_US;
}

void init_paging() {
    unsigned int i;
    unsigned long pfn = 0;
    pte_t *pte = 0;
    unsigned long *pgtb_addr = 0;
    void *alloc_from_bootmem(unsigned long size, char *title);

    // 在multiboot.S是已经初始化了BOOT_INIT_PAGETBL_CNT个页
    // 这里接着初始化剩余的页
    // 最大限制内存1G
    for (pfn = pa2pfn(BOOT_INIT_PAGETBL_CNT << 22); pfn < bootmem_data.max_pfn; ++pfn) {
        unsigned long ti = pfn % PAGE_PTE_CNT;
        unsigned long page_addr = (unsigned long)pfn2pa(pfn);
        if (ti == 0) {
            pgtb_addr = (unsigned long *)alloc_from_bootmem(PAGE_SIZE, "paging");
            if (0 == pgtb_addr) {
                panic("no pages for paging...");
            }

            memset((void *)pgtb_addr, 0, PAGE_SIZE);

            init_pgd[get_npde(page_addr)] = (pde_t)((unsigned long)va2pa(pgtb_addr) | PAGE_P | PAGE_WR);
        }

        pte = ((pte_t *)pgtb_addr) + ti;
        *pte = (pte_t)(page_addr | PAGE_P | PAGE_WR);
    }

    // paging for kernel space
    unsigned long delta = get_npde(PAGE_OFFSET);
    for (i = delta; i < PDECNT_PER_PAGE; ++i) {
        init_pgd[i] = init_pgd[i - delta];
        init_pgd[i - delta] = 0;
    }

    // 接下来为显存建立页映射
    unsigned long vram_phys_addr = system.vbe_phys_addr;
    for (int pde_inx = 0; pde_inx < get_npde(VRAM_VADDR_SIZE); pde_inx++) {
        pgtb_addr = (unsigned long *)(alloc_from_bootmem(PAGE_SIZE, "vrampaging"));
        if (0 == pgtb_addr) {
            panic("no pages for paging...");
        }
        // 后续要初始化，所以此处不用memset
        // memset((void *)pgtb_addr, 0, PAGE_SIZE);
        init_pgd[get_npde(VRAM_VADDR_BASE) + pde_inx] = (pde_t)((unsigned long)va2pa(pgtb_addr) | PAGE_P | PAGE_WR);

        for (int pte_inx = 0; pte_inx < PTECNT_PER_PAGE; pte_inx++) {
            pgtb_addr[pte_inx] = vram_phys_addr | PAGE_P | PAGE_WR;
            vram_phys_addr += PAGE_SIZE;
        }
    }

    // paging for user space
    extern void sysexit();
    set_page_shared(sysexit);

    LoadCR3(va2pa(init_pgd));

    // // 测试显存
    // for (int i = 0; i < system.x_resolution * (system.y_resolution - 32); i++) {
    //     unsigned long *vram = (unsigned long *)VRAM_VADDR_BASE;
    //     vram[i] = 0x000000FF;
    // }

    // while (1) {
    //     u16 lineH = 32;
    //     unsigned long *vram = (unsigned long *)VRAM_VADDR_BASE;
    //     int sep = system.x_resolution * (system.y_resolution - lineH);
    //     for (int i = 0; i < sep; i++) {
    //         vram[i] = vram[i + system.x_resolution * lineH];
    //     }

    //     unsigned int long color = 0x0000FF;
    //     color = (vram[0] == 0x0000FF ? 0x00FF00 : 0x0000FF);

    //     for (int i = sep; i < sep + system.x_resolution * lineH; i++) {
    //         vram[i] = color;
    //     }
    // }
}

extern void init_ttys();

void init_mm() {
    printk("init bootmem alloc...\n");
    extern void init_bootmem();
    init_bootmem();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);
    printk("init global paging...\n");
    init_paging();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    printk("init buddy system...\n");
    extern void init_buddy_system();
    init_buddy_system();
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);

    printk("init kmem caches...\n");
    extern void init_kmem_caches();
    init_kmem_caches();
    printk("memory init finished...\n");
    boot_delay(DEFAULT_BOOT_DELAY_TICKS);
}

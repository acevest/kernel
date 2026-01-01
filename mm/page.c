/*
 *--------------------------------------------------------------------------
 *   File Name: page.c
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sun Jan 24 15:14:24 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */

#include <assert.h>
#include <mm.h>
#include <page.h>
#include <printk.h>
#include <sched.h>
#include <types.h>

void do_no_page(void* addr) {
    pde_t* page_dir = (pde_t*)pa2va(current->cr3);
    pte_t* page_tbl = 0;

    unsigned long page = (unsigned long)page2va(alloc_one_page(0));
    assert(page != 0);

    int npde = get_npde(addr);
    int npte = get_npte(addr);

    if (page_dir[npde] == 0) {
        page_tbl = (pte_t*)page2va(alloc_one_page(0));
        assert(page_tbl != 0);

        memset((void*)page_tbl, 0, PAGE_SIZE);

        page_dir[npde] = va2pa(page_tbl) | PAGE_P | PAGE_WR | PAGE_US;
    }

    page_tbl = (pte_t*)pa2va(PAGE_ALIGN(page_dir[npde]));
    page_tbl[npte] = va2pa(page) | PAGE_P | PAGE_WR | PAGE_US;

    load_cr3(current);
}

void do_wp_page(void* addr) {
    // printk("%s   addr %08x current %08x\n", __func__, (unsigned long)addr, current);
    if ((unsigned long)addr >= PAGE_OFFSET) {
        panic("%s invalid addr", __func__);
    }

    int npde = get_npde(addr);
    int npte = get_npte(addr);

    pde_t* page_dir = (pde_t*)pa2va(current->cr3);
    pde_t* pde = page_dir + npde;

    // 如果是因为PDE被写保护
    if (*pde & PDE_RW == 0) {
        // 1. 分配一个页表
        unsigned long newtbl = (unsigned long)page2va(alloc_one_page(0));
        assert(newtbl != 0);

        // 2. 拷贝页表
        pte_t* oldtbl = pa2va(PAGE_ALIGN(page_dir[npde]));
        memcpy((void*)newtbl, (void*)oldtbl, PAGE_SIZE);

        // 3. 对所有PTE加上写保护
        pte_t* pte = (pte_t*)newtbl;
        for (int i = 0; i < PTECNT_PER_PAGE; i++) {
            *pte &= ~PTE_RW;
            pte++;
        }

        // 4. 记下原PDE的权限
        unsigned long flags = PAGE_FLAGS(*pde);

        // 5. 设置新的PDE
        *pde = va2pa(newtbl);
        *pde |= flags;

        // 6. 解除PDE的写保护
        *pde |= PDE_RW;
    }

    pte_t* page_tbl = pa2va(PAGE_ALIGN(page_dir[npde]));
    pte_t* pte = page_tbl + npte;

    // 如果PTE的位置被写保护
    if (*pte & PTE_RW == 0) {
        // 1. 分配一个页表
        unsigned long newaddr = (unsigned long)page2va(alloc_one_page(0));
        assert(newaddr != 0);

        // 2. 拷贝页表
        pte_t* oldaddr = pa2va(PAGE_ALIGN(page_dir[npde]));
        memcpy((void*)newaddr, (void*)oldaddr, PAGE_SIZE);

        // 3. 解除PTE的写保护
        *pte |= PTE_RW;
    }

#if 0
    int npde = get_npde(addr);
    int npte = get_npte(addr);

    pde_t *page_dir = (pde_t *)pa2va(current->cr3);
    pte_t *page_tbl = pa2va(PAGE_ALIGN(page_dir[npde]));

    unsigned long wp_pa_addr = PAGE_ALIGN(page_tbl[npte]);

    page_t *page = pa2page(wp_pa_addr);
    if (page->count > 0) {
        page->count--;
        unsigned long flags = PAGE_FLAGS(page_tbl[npte]);
        unsigned long wp_va_addr = (unsigned long)pa2va(wp_pa_addr);
        unsigned long newtbl = (unsigned long) page2va(alloc_one_page(0));
        assert(newtbl != 0);

        memcpy((void *)newtbl, (void *)wp_va_addr, PAGE_SIZE);

        page_tbl[npte] = va2pa(newtbl) | flags;
    }

    page_tbl[npte] |= PAGE_WR;
#endif
#if 0
    page_tbl[npte] |= PAGE_US;
    page_dir[npde] |= PAGE_WR;
    page_dir[npde] |= PAGE_US;
#endif

    load_cr3(current);
}

void page_map(vaddr_t vaddr, paddr_t paddr, uint32_t flags) {
    assert(PAGE_ALIGN(vaddr) == vaddr);
    assert(PAGE_ALIGN(paddr) == paddr);
    assert(PAGE_FLAGS(flags) == flags);

    int npde = get_npde(vaddr);
    int npte = get_npte(vaddr);

    // 获取页目录地址
    pde_t* pgd = (pde_t*)get_pgd();

    pte_t* pgt = 0;
    pde_t pde = pgd[npde];
    if (pde & PAGE_P) {
        pgt = (pte_t*)pa2va(PAGE_ALIGN(pde));
    } else {
        // 分配页表
        pgt = (pte_t*)page2va(alloc_one_page(0));

        // 清空页表
        memset(pgt, 0, PAGE_SIZE);

        // 页目录指向页表
        pgd[npde] = PAGE_ALIGN(va2pa(pgt)) | flags;
    }

    // 页表指向物理地址
    pgt[npte] = PAGE_ALIGN(paddr) | flags;

    asm volatile("invlpg (%0)" : : "r"(vaddr));
}

typedef uint32_t pfn_t;

void set_pte_pfn(vaddr_t vaddr, pfn_t pfn, uint32_t flags) {
    assert(PAGE_ALIGN(vaddr) == vaddr);
    assert(PAGE_FLAGS(flags) == flags);
    assert(pfn >= 0);
    assert(pfn < (1 << (32 - PAGE_SHIFT)));  // 最大4G空间

    int npde = get_npde(vaddr);
    int npte = get_npte(vaddr);

    pde_t* pgd = (pde_t*)get_pgd();
    assert(pgd != 0);

    pde_t pde = pgd[npde];
    assert(pde & PAGE_P == PAGE_P);

    pte_t* pgt = (pte_t*)pa2va(PAGE_ALIGN(pde));
    pte_t pte = pgt[npte];
    assert(pte == 0);

    pgt[npte] = (pfn << PAGE_SHIFT) | flags;
}

void set_pte_paddr(vaddr_t vaddr, paddr_t paddr, uint32_t flags) {
    assert(PAGE_ALIGN(vaddr) == vaddr);
    assert(PAGE_ALIGN(paddr) == paddr);
    assert(PAGE_FLAGS(flags) == flags);
    set_pte_pfn(vaddr, paddr >> PAGE_SHIFT, flags);
}

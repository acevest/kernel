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
#include <task.h>
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

char *paging_page = "paging";

void init_paging() {
    unsigned int i;
    unsigned long pfn = 0;
    pte_t *pg_table = 0;
    void *alloc_from_bootmem(unsigned long size, char *title);

    // 在multiboot.S是已经初始化了BOOT_INIT_PAGETBL_CNT个页
    // 这里接着初始化剩余的页
    // 最大限制内存1G
    for (pfn = pa2pfn(BOOT_INIT_PAGETBL_CNT << 22); pfn < bootmem_data.max_pfn; ++pfn) {
        unsigned long npte = pfn % PAGE_PTE_CNT;
        unsigned long page_addr = (unsigned long)pfn2pa(pfn);
        if (npte == 0) {
            pg_table = (pte_t *)alloc_from_bootmem(PAGE_SIZE, paging_page);
            if (0 == pg_table) {
                panic("no pages for paging...");
            }

            memset((void *)pg_table, 0, PAGE_SIZE);

            init_pgd[get_npde(page_addr)] = (pde_t)((unsigned long)(pg_table) | PAGE_P | PAGE_WR);
        }

        pg_table[npte] = (pte_t)(page_addr | PAGE_P | PAGE_WR);
    }

    // paging for kernel space
    // 在此处让内核空间对[0, MAX_SUPT_PHYMM_SIZE]物理内存也作直接映射
    int kernel_npde_base = get_npde(PAGE_OFFSET);
    for (i = kernel_npde_base; i < PDECNT_PER_PAGE; ++i) {
        init_pgd[i] = init_pgd[i - kernel_npde_base];
    }

    // 接下来还有部分内核空间需要处理，例如 VRAM_VADDR FIXED_MAP_VADDR 等
    // 这部分空间也需要给他们分配固定的页表
    // 这部分页表在pgd里的pde就不再允许修改了
    // 这样，无论内核空间映射如何变化，这部分空间所有进程都能共享到变化
    for (i = kernel_npde_base; i < PDECNT_PER_PAGE; ++i) {
        if(0 != init_pgd[i]) {
            continue;
        }

        // 分配一个页表
        pte_t *pg_table = (pte_t *)alloc_from_bootmem(PAGE_SIZE, paging_page);
        if(0 == pg_table) {
            panic("no pages for paging...");
        }

        // 清空页表
        memset((void *)pg_table, 0xAC, PAGE_SIZE);

        // 把页表地址填入pgd
        init_pgd[i] = (pde_t)((unsigned long)(pg_table) | PAGE_P | PAGE_WR);
    }


    // 建立完内核空间的页映射，需要清空用户空间的映射
    for (i = 0; i < kernel_npde_base; ++i) {
        init_pgd[i] = 0;
    }



#if 0
    // 接下来为显存建立页映射
    unsigned long vram_phys_addr = system.vbe_phys_addr;
    printk("vram_phys_addr: 0x%x\n", vram_phys_addr);
    for (int pde_inx = 0; pde_inx < get_npde(VRAM_VADDR_SIZE); pde_inx++) {
        pgtb_addr = (unsigned long *)(alloc_from_bootmem(PAGE_SIZE, "vrampaging"));
        if (0 == pgtb_addr) {
            panic("no pages for paging...");
        }
        // 后续要初始化，所以此处不用memset
        // memset((void *)pgtb_addr, 0, PAGE_SIZE);
        init_pgd[get_npde(VRAM_VADDR_BASE) + pde_inx] = (pde_t)((unsigned long)(pgtb_addr) | PAGE_P | PAGE_WR);

        for (int pte_inx = 0; pte_inx < PTECNT_PER_PAGE; pte_inx++) {
            pgtb_addr[pte_inx] = vram_phys_addr | PAGE_P | PAGE_WR;
            vram_phys_addr += PAGE_SIZE;
        }
    }

    // paging for user space
    extern void sysexit();
    set_page_shared(sysexit);

    LoadCR3(va2pa(init_pgd));

    // 测试显存
    for (int i = 0; i < system.x_resolution * (system.y_resolution - 32); i++) {
        unsigned long *vram = (unsigned long *)VRAM_VADDR_BASE;
        // 仅为32bit色深
        // 在内存中 [B, G, R, A] 因为x86是小端序 所以实际是 ARGB 顺序
        vram[i] = 0x000000FF;
    }
#endif

#if 0
    bool flag = false;
    while (1) {
        u16 lineH = 32;
        unsigned long *vram = (unsigned long *)VRAM_VADDR_BASE;
        int sep = system.x_resolution * (system.y_resolution - lineH);
        for (int i = 0; i < sep; i++) {
            vram[i] = vram[i + system.x_resolution * lineH] | 0x00FF0000;
        }

        unsigned int long color = 0x0000FF;
        color = (vram[0] == 0x0000FF ? 0x00FF00 : 0x0000FF);

        for (int i = sep; i < sep + system.x_resolution * lineH; i++) {
            vram[i] = color;
        }

        for(int i=0; i<32; i++) {
            for(int j=0; j<64; j++) {
                unsigned int long c = vram[j*system.x_resolution+i];
                c = flag ? 0x00FFFFFF : 0x000000FF;
                // c &= 0x00FFFFFF;
                vram[j*system.x_resolution+i] = c;
            }
        }
        flag = !flag;

        for(int i=0; i<10*1000*10000; i++) {
            asm("nop");
        }
    }
#endif
}

extern void init_ttys();

kmem_cache_t *g_vma_kmem_cache = NULL;

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

    g_vma_kmem_cache = kmem_cache_create("vma", sizeof(vm_area_t), 4);
    assert(g_vma_kmem_cache != NULL);
}

vm_area_t *vma_alloc() {
    vm_area_t *vma = kmem_cache_alloc(g_vma_kmem_cache, 0);
    vma->vm_bgn = 0;
    vma->vm_end = 0;
    vma->vm_flags = 0;
    vma->vm_next = NULL;
    return vma;
}

void vma_insert(task_t *tsk, vm_area_t *vma_new) {
    //
    vm_area_t **p = &tsk->vma_list;

    while (*p != NULL) {
        vm_area_t *v = *p;

        assert(v->vm_bgn < v->vm_end);
        assert(vma_new->vm_bgn < vma_new->vm_end);
        assert(vma_new->vm_bgn < v->vm_bgn || vma_new->vm_bgn > v->vm_end);
        assert(vma_new->vm_end < v->vm_bgn || vma_new->vm_end > v->vm_end);

        if (vma_new->vm_end < v->vm_bgn) {
            break;
        }

        p = &v->vm_next;
    }

    vma_new->vm_next = *p;
    *p = vma_new;
}

vm_area_t *vma_find(task_t *tsk, vm_area_t *vma, uint32_t addr) {
    vm_area_t **p = &tsk->vma_list;
    while (*p != NULL) {
        vm_area_t *v = *p;

        if (addr <= v->vm_end) {
            break;
        }

        p = &v->vm_next;
    }

    return *p;
}

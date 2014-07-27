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

#include <page.h>
#include <types.h>
#include <sched.h>
#include <assert.h>
#include <printk.h>
#include <mm.h>

void do_no_page(void *addr)
{
    printk("%s   addr %08x\n", __func__, (unsigned long)addr);
    pde_t *pde = (pde_t *)current->cr3;
    pte_t *pte;
    unsigned long page = alloc_one_page(0);
    page = va2pa(page);
    if(page == 0)
        panic("failed alloc page");

    int npde = get_npd(addr);
    int npte = get_npt(addr);

    if(PAGE_ALIGN(pde[npde]) == 0)
    {
        pte = (pte_t*) alloc_one_page(0);
        memset((void*)pte, 0, PAGE_SIZE);
        if(pte == NULL)
            panic("failed alloc pte");

        pte[npte] = (pte_t) page | PAGE_P | PAGE_WR | PAGE_US;
        pde[npde] = va2pa(pte) | PAGE_P | PAGE_WR | PAGE_US;
    }
    else
    {
        pte = (pte_t*)(PAGE_ALIGN(pde[npde]));
        pte = pa2va(pte);
        pte[npte] = (u32) page | PAGE_P | PAGE_WR | PAGE_US;
    }

    load_cr3(current);
}


void do_wp_page(void *addr)
{
    //printk("%s   addr %08x\n", __func__, (unsigned long)addr);
    int npde = get_npd(addr);
    int npte = get_npt(addr);

    //unsigned long *pd = (u32 *)pa2va(current->cr3);
    //unsigned long *pd = (u32 *)va2pa(current->cr3);
    unsigned long *pd = (u32 *)(current->cr3);
    unsigned long *pt = NULL;

    pt = pa2va(PAGE_ALIGN(pd[npde]));

    unsigned long src, dst;

    src = pt[npte];

    page_t *page = pa2page(src);

    if(page->count > 0)
    {
        unsigned long flags = PAGE_FLAGS(src);

        page->count--;

        src = (unsigned long) pa2va(PAGE_ALIGN(src));

        dst = alloc_one_page(0);

        if(0 == dst)
            panic("out of memory");

        dst = va2pa(dst);

        pt[npte] = dst | flags;
        pt[npte] |= PAGE_WR;
        pd[npde] |= PAGE_WR;

        dst = (unsigned long)pa2va(PAGE_ALIGN(dst));

        memcpy((void *)dst, (void *)src, PAGE_SIZE);
    }
    else
    {
        pd[npde] |= PAGE_WR;
        pt[npte] |= PAGE_WR;
        //pd[npde] |= PAGE_US;
        //pt[npte] |= PAGE_US;
    }

    load_cr3(current);
}

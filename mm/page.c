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
#include <bits.h>
#include <types.h>
#include <sched.h>
#include <assert.h>
#include <printk.h>
#include <init.h>
#include <mm.h>



#define get_page_from_list(pList) list_entry(pList, Page, list)
#define add_page2list(page, order) \
        list_add(&page->list, &freeArea[order].freeList)

FreeArea freeArea[MAX_OLD_ORDER];

#if 1
void    do_no_page(void *addr)
{
    //printk("%s ", __FUNCTION__);
    u32    *pde = (u32*)pa2va(current->cr3);
    u32    *pte;
    void    *page = get_phys_pages(1);
    if(page == NULL)
        panic("failed alloc page");

    int npde = ((u32)addr)>>22;
    int npte = (((u32)addr)>>12) & 0x3FF;

    if((pde[npde] & 0xFFFFF000)== 0)
    {
        //printk("*a*\n");
        pte = (u32 *) pa2va(get_phys_pages(1));
        memset((void*)pte, 0, PAGE_SIZE);
        if(pte == NULL)
            panic("failed alloc pte");

        pte[npte] = (u32) page | 7;
        pde[npde] = va2pa(pte) | 7;
    }
    else
    {
        //printk("*b* : %08x\n", page);
        //printk("*b* : %08x %08x\n", pde[npde], page);
        pte = (u32*)(pde[npde] & 0xFFFFF000);
        pte = pa2va(pte);
        pte[npte] = (u32) page | 7;
    }
    load_cr3(current);
#if 0
    u32    *pde = (u32*)pa2va(current->cr3);
    u32    *pte;
    void    *page = (void*)va2pa(kmalloc_old(PAGE_SIZE));
    if(page == NULL)
        panic("failed alloc page");

    int npde = ((u32)addr)>>22;
    int npte = (((u32)addr)>>12) & 0x3FF;

    if(pde[npde] == 0)
    {
        printk("*a*");
        pte = (u32 *) kmalloc_old(PAGE_SIZE);
        memset((void*)pte, 0, PAGE_SIZE);
        if(pte == NULL)
            panic("failed alloc pte");

        pte[npte] = (u32) page | 7;
        pde[npde] = va2pa(pte) | 7;
    }
    else
    {
        printk("*b*");
        pte = pde[npde] & 0xFFFFF000;
        pte = pa2va(pte);
        pte[npte] = (u32) page | 7;
    }
#endif
}


void do_wp_page(void *addr)
{
    int npde = get_npd(addr);
    int npte = get_npt(addr);

    unsigned long *pd = (u32 *)pa2va(current->cr3);
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

        pt[npte] = dst | flags;

        dst = (unsigned long)pa2va(PAGE_ALIGN(dst));

        memcpy((void *)dst, (void *)src, PAGE_SIZE);
    }
    else
    {
        pt[npte] |= PAGE_WR;
    }

    load_cr3(current);
}

inline pPage __old_alloc_pages(unsigned int order, unsigned int alloc_order)
{
    assert(0 <= order && order<MAX_OLD_ORDER);
    assert(!list_empty(&freeArea[order].freeList));
    assert(alloc_order <= order);

    pListHead pl = freeArea[order].freeList.next;
    list_del_init(pl);
    pPage    page = get_page_from_list(pl);

    if(order == alloc_order)
    {
        page->order = alloc_order;
        page->count = 0;
        assert((page->mapNR%(1UL<<(order))) == 0);
        return page;
    }

    assert(order >= 1);

    // 把这个页分成两半挂到小一级的队列上
    add_page2list(page, order-1);
#if 0
    if(page->mapNR == 0)
    {
        printk("[>1<%08x %d %d]\n", page, order, alloc_order);
        while(1);
    }
#endif
    assert((page->mapNR%(1UL<<(order-1))) == 0);
    page += (1 << (order-1));
    assert((page->mapNR%(1UL<<(order-1))) == 0);
#if 0
    if(page->mapNR == 0)
    {
        printk("[>2<%08x %d %d]\n", page, order, alloc_order);
        while(1);
    }
#endif
    add_page2list(page, order-1);

    //int j=2000000;while(j--);
    return __old_alloc_pages(order-1, alloc_order);
}

pPage    old_alloc_pages(unsigned int order)
{
    if(order<0 || order>=MAX_OLD_ORDER)
        return NULL;

    int i;
    for(i=order; i<MAX_OLD_ORDER; i++)
    {
        if(list_empty(&freeArea[i].freeList))
            continue;

        return    __old_alloc_pages(i, order);
    }
    return NULL;
}


pPage    get_buddy_page(pPage page, unsigned int order)
{
    assert(0<=order && order<MAX_OLD_ORDER);
    assert(page != NULL);

    //printk("mapnr: %d order:%d %d\n", page->mapNR, order, 1UL<<order);
    assert((page->mapNR%(1UL<<order)) == 0);

    int off = 1<<(order+0);
    int div = 1<<(order+1);
    int nr = page->mapNR;

    pListHead pos, tmp;
    pPage p;

    nr += ((nr+off)/div == nr/div) ? off : -off;

    list_for_each_safe(pos, tmp, &freeArea[order].freeList)
    {
        p = get_page_from_list(pos);
        if(p->mapNR == nr)
        {
            list_del_init(&p->list);
            p->order = order;
            return p;
        }
    }


    return NULL;
}

void    old_free_pages(pPage page)
{
    assert(page != NULL);
    unsigned int order = page->order;
    assert(0<=order && order<MAX_OLD_ORDER);


    // buddy page
    //printk("[%d %d]", page->mapNR, page->order);
    assert((page->mapNR % (1UL<<order)) == 0);
    pPage bpage = get_buddy_page(page, order);
    //printk("<%d %d>", bpage->mapNR, bpage->order);
    

    if(bpage == NULL || order == MAX_OLD_ORDER-1)
    {
        add_page2list(page, order);
    }
    else
    {
        //list_del_init(&bpage->list);
        page = (page->mapNR > bpage->mapNR) ? bpage : page;
        page->order += 1;
        assert((page->mapNR%(1UL<<page->order)) == 0);
#if 0
        if((page->mapNR%(1UL<<page->order)) != 0)
        {
            printk("*%d %d*", page->mapNR, 1UL<<page->order);
            panic("bug");
        }
#endif
        old_free_pages(page);
    }
}

#else

inline pPage __old_alloc_pages(unsigned int order, unsigned int alloc_order)
{
    assert(0 <= order && order<MAX_OLD_ORDER);
    assert(!list_empty(&freeArea[order].freeList));
    assert(alloc_order <= order);

    pListHead pl = freeArea[order].freeList.next;
    list_del_init(pl);
    pPage    pg = list_entry(pl, Page, list);
    change_bit(pg->mapNR>>(order+1), (unsigned long *)freeArea[order].map);

    if(order == alloc_order)
    {
        return pg;
    }

    // 把这个页分成两半挂到小一级的队列上
    assert(order >= 1);
    list_add(&pg->list, &freeArea[order-1].freeList);
    pg += (1 << (order-1));
    list_add(&pg->list, &freeArea[order-1].freeList);

    return __old_alloc_pages(order-1, alloc_order);
}

pPage old_alloc_pages(unsigned int order)
{
    if(order >= MAX_OLD_ORDER)
        return NULL;

    int i;
    for(i=order; i<MAX_OLD_ORDER; i++)
    {
        if(list_empty(&freeArea[i].freeList))
            continue;

        return __old_alloc_pages(i, order);
    }

    return NULL;
}


void old_free_pages(pPage page, unsigned int order)
{
    assert(0<=order &&  order<MAX_OLD_ORDER);
    assert(page != NULL);


    
    int nr = page->mapNR>>(order+1);
    //printk("#########%d %d\n",page->mapNR, variable_test_bit( nr, (unsigned long *)freeArea[order].map));
    if(order == MAX_OLD_ORDER -1
    || !variable_test_bit( nr, (unsigned long *)freeArea[order].map))
    {
        change_bit( nr, (unsigned long *)freeArea[order].map);
        list_add(&page->list, &freeArea[order].freeList);
    }
    else
    {
        
        pListHead pos, tmp;

        int offset = 1<<order;
        nr = page->mapNR;
        printk("------------%d %d ", offset, nr);
        nr += (((nr+offset)>>(order)) == (nr>>(order)))
             ? offset : -offset;
        printk("%d\n", nr);
        list_for_each_safe(pos, tmp, &freeArea[order].freeList)
        {
            printk("[%d] ",list_entry(pos, Page, list)->mapNR);
            if(list_entry(pos, Page, list)->mapNR == nr)
                break;
        }

        assert(pos != &freeArea[order].freeList);
        list_del_init(pos);

        nr = page->mapNR>>(order+2);
        change_bit( nr, (unsigned long *)freeArea[order+1].map);
        pPage bpage = get_page_from_list(pos);
        page = (page->mapNR > bpage->mapNR)? bpage: page;
        list_add(&page->list, &freeArea[order+1].freeList);
    }
}
#endif

void disp_free_area()
{
    int i;
    for(i=0; i<MAX_OLD_ORDER; i++)
    {
        pListHead pos,tmp;
        int count = 0;

        list_for_each_safe(pos, tmp, &freeArea[i].freeList)
        {
            count++;
            //printk("%d ", list_entry(pos, Page, list)->mapNR);
            //printk("%08x %08x %08x %08x\n",pos, pos->prev, tmp,
            //        &(freeArea[i].freeList));
        }
        printk("[%d %d]",i, count);
    }
    printk("\n");
    int j=100000;
    //while(j--);
}

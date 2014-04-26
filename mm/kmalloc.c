/*
 *--------------------------------------------------------------------------
 *   File Name: kmalloc_old.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sat Jan 30 12:15:51 2010
 * 
 * Description: 现在的版本实现得非常简单，简陋。不能分配大于32*4K的内存
 *         另外小于32*4K的都按32*4K分配
 *         以后再来重写这里吧
 *--------------------------------------------------------------------------
 */
#include <page.h>
#include <types.h>
#include <assert.h>
#include <system.h>

static    get_order(size_t size)
{
    //printk("size:%08x ", size);
    size = ALIGN(size, PAGE_SIZE);
    //printk(" %08x\n", size);
    int i;
    int n = size>>PAGE_SHIFT;
    for(i=0; i<MAX_OLD_ORDER; i++)
    {
        if(n<=(1UL<<i))
            break;
    }

    //printk("i:%d\n", i);
    return i;
}
void    *kmalloc_old(size_t size)
{
    assert(0<size && size<=32*PAGE_SIZE);
    int    order = get_order(size);
    void    *p;
    pPage page = old_alloc_pages(order);

    if(page == NULL)
        return NULL;

    //printk("kmalloc_old:%08x %08x ", page->mapNR, page->mapNR<<PAGE_SHIFT);
    p = (void*)pa2va(page->mapNR<<PAGE_SHIFT);
    
    //printk("kmalloc_old: %08x\n", p);

    return p;
}


void    kfree_old(void *p)
{
    assert(p != NULL);

    pPage page = system.page_map;
    //printk("kfree_old:%08x %08x %08x ", p, va2pa(p));
    page += ((unsigned long)va2pa(p)>>PAGE_SHIFT);
    //printk("%08x\n", page->mapNR);
    old_free_pages(page);
}

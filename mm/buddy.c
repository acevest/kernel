/*
 * ------------------------------------------------------------------------
 *   File Name: buddy.c
 *      Author: Zhao Yanbai
 *              Fri Apr  4 19:25:02 2014
 * Description: none
 * ------------------------------------------------------------------------
 */
#include <mm.h>

#define MAX_ORDER       10
struct buddy_system
{
    page_t      *page_map;
    free_area_t free_area[MAX_ORDER + 1];
};

struct buddy_system buddy_system;

int buddy_is_free(page_t *page, unsigned int order)
{
    if(PagePrivate(page) && page->private == order)
        return 1;

    return 0;
}

void __free_pages(page_t *page, unsigned int order)
{
    if(order > MAX_ORDER)
        return ;

    page_t *buddy = 0;
    page_t *base  = buddy_system.page_map;
    unsigned long page_inx  = page - base;
    while(order < MAX_ORDER)
    {
        unsigned long buddy_inx = page_inx ^ (1UL << order);
        buddy = base + buddy_inx;
        if(!buddy_is_free(buddy, order)) {
            break;
        }

        list_del(&buddy->lru);
        buddy_system.free_area[order].free_count--;

        ClearPagePrivate(buddy);
        buddy->private = 0;

        page_inx &= buddy_inx;

        order++;
    }

    page_t *p  = base + page_inx;
    p->private = order;
    p->index   = page_inx;
    SetPagePrivate(p);
    list_add(&(p->lru), &(buddy_system.free_area[order].free_list));
    buddy_system.free_area[order].free_count++;
}

void dump_buddy_system()
{
    unsigned long i;
    for(i=0; i<MAX_ORDER+1; ++i)
    {
        printk("order %2d free_count %d ", i, buddy_system.free_area[i].free_count);

        if(buddy_system.free_area[i].free_count < 100)
        {
            list_head_t *p;
            page_t *page;
            printk("pfn:");
            list_for_each(p, &buddy_system.free_area[i].free_list)
            {
                page = list_entry(p, page_t, lru);
                printk(" %d", page->index);
            }
        }

        printk("\n");
    }

}

void init_buddy_system()
{
    page_t *page;
    unsigned long i;
    unsigned long pfn_cnt = bootmem_total_pages();

    // init free area
    memset(&buddy_system, 0, sizeof(buddy_system));

    for(i=0; i<MAX_ORDER+1; ++i)
    {
        INIT_LIST_HEAD(&(buddy_system.free_area[i].free_list));
    }


    // init page map
    unsigned long page_map_size = pfn_cnt*sizeof(page_t);
    buddy_system.page_map = alloc_bootmem(page_map_size, PAGE_SIZE);
    if(0 == buddy_system.page_map) {
        printk("can not go on playing...\n");
        while(1);
    }

    printk("page_map begin %08x end %08x\n", buddy_system.page_map, buddy_system.page_map + pfn_cnt);

    for(i=0; i<pfn_cnt; ++i)
    {
        page = buddy_system.page_map + i;
        page->private = 0;
        page->index   = i;
        INIT_LIST_HEAD(&(page->lru));
        ClearPagePrivate(page);
    }

    // get free pages from bootmem
    for(i=0; i<pfn_cnt; ++i)
    {
        page = buddy_system.page_map + i;

        if(BOOTMEM_PAGE_FREE == bootmem_page_state(i))
        {
            // free to buddy system
            __free_pages(page, 0);
        }
    }

    // free bootmem bitmap pages to buddy system
    // ...

    dump_buddy_system();
}

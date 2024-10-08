/*
 * ------------------------------------------------------------------------
 *   File Name: buddy.c
 *      Author: Zhao Yanbai
 *              Fri Apr  4 19:25:02 2014
 * Description: none
 * ------------------------------------------------------------------------
 */
#include <irq.h>
#include <list.h>
#include <mm.h>
#include <string.h>
#include <sysctl.h>

struct buddy_system {
    page_t *page_map;
    page_t *page_map_end;
    free_area_t free_area[MAX_ORDER];
};

struct buddy_system buddy_system;

int buddy_is_free(page_t *page, unsigned int order) {
    if (PagePrivate(page) && page->private == order) return 1;

    return 0;
}

page_t *_va2page(unsigned long addr) {
    page_t *page = buddy_system.page_map + va2pfn(addr);

    assert(page >= buddy_system.page_map);
    // assert(page < buddy_system.page_map_end);
    if (page >= buddy_system.page_map_end) {
        printk("buddy_system.page_map %08x buddy_system.page_map_end %08x\n", buddy_system.page_map,
               buddy_system.page_map_end);
        printk("error %s page %08x addr %08x\n", __func__, page, addr);
        panic("error");
    }

    return page;
}
page_t *_pa2page(unsigned long paddr) {
    unsigned long vaddr = (unsigned long)pa2va(paddr);
    // printk("%s paddr %08x vaddr %08x\n", __func__, paddr, vaddr);
    return va2page(vaddr);
}

void *page2va(page_t *page) { return pfn2va((page)-buddy_system.page_map); }
void *page2pa(page_t *page) { return pfn2pa((page)-buddy_system.page_map); }

page_t *__alloc_pages(unsigned int order) {
    //
    page_t *page = 0;
    free_area_t *area;
    unsigned int select_order;
    unsigned int i;

    // 找到首个满足 >= order的page块
    for (select_order = order; select_order < MAX_ORDER; ++select_order) {
        area = buddy_system.free_area + select_order;
        if (!list_empty(&(area->free_list))) {
            goto found;
        }
    }

    return 0;

found:
    // 从链表取下这个连续的page块
    page = list_entry(area->free_list.next, page_t, lru);
    list_del(&(page->lru));
    ClearPagePrivate(page);
    page->private = 0;
    area->free_count--;

    // 二分这个page块，把空闲的挂到对应的order队列上
    // 直到得到order大小的page块
    while (select_order > order) {
        area--;
        select_order--;
        unsigned long buddy_offset = 1UL << select_order;

        page_t *buddy_page = page + buddy_offset;
        list_add(&(buddy_page->lru), &(area->free_list));
        area->free_count++;
        buddy_page->private = select_order;
        SetPagePrivate(buddy_page);
    }

    // 初始化这一连续的page块
    for (i = 0; i < (1UL << order); ++i) {
        page_t *p = page + i;
        p->head_page = page;
        p->order = order;
    }

    page->count = 0;
    page->buffers = NULL;
    page->cache = NULL;
    page->count = 0;
    page->flags = 0;
    page->hash_next = NULL;
    page->index = 0;
    page->inuse = 0;
    page->freelist = NULL;
    INIT_LIST_HEAD(&page->lru);
    INIT_LIST_HEAD(&page->list);
    page->mapping = NULL;
    // page->private = 0;

    return page;
}

page_t *alloc_pages(unsigned int gfp_mask, unsigned int order) {
    // gfp_mask
    // ...

    unsigned long flags;
    irq_save(flags);
    page_t *page = __alloc_pages(order);
    irq_restore(flags);

    // unsigned long addr = (unsigned long)page2va(page);
    // return addr;

    return page;
}

void __free_pages(page_t *page, unsigned int order) {
    if (order > MAX_ORDER) return;

    page_t *buddy = 0;
    page_t *base = buddy_system.page_map;
    unsigned long page_inx = page - base;
    while (order < (MAX_ORDER - 1)) {
        unsigned long buddy_inx = page_inx ^ (1UL << order);
        buddy = base + buddy_inx;
        if (!buddy_is_free(buddy, order)) {
            break;
        }

        list_del(&buddy->lru);
        buddy_system.free_area[order].free_count--;

        ClearPagePrivate(buddy);
        buddy->private = 0;

        page_inx &= buddy_inx;

        order++;
    }

    page_t *p = base + page_inx;
    p->private = order;
    p->index = page_inx;
    SetPagePrivate(p);
    list_add(&(p->lru), &(buddy_system.free_area[order].free_list));
    buddy_system.free_area[order].free_count++;
}

void free_pages(unsigned long addr) {
    if (!valid_va(addr)) {
        BUG_ON(!valid_va(addr));
    }

    page_t *page = va2page(addr);

    unsigned long flags;
    irq_save(flags);
    __free_pages(page, page->order);
    irq_restore(flags);
}

void dump_buddy_system() {
    unsigned long i;
    for (i = 0; i < MAX_ORDER; ++i) {
        printk("order %2d free_count %d ", i, buddy_system.free_area[i].free_count);

        if (buddy_system.free_area[i].free_count < 1000) {
            list_head_t *p;
            page_t *page;
            printk("pfn:");
            list_for_each(p, &buddy_system.free_area[i].free_list) {
                page = list_entry(p, page_t, lru);
                printk(" %d", page->index);
            }
        }

        printk("\n");
    }
}

void init_buddy_system() {
    page_t *page;
    unsigned long i;
    unsigned long pfn_cnt = bootmem_max_pfn();

    // init free area
    memset(&buddy_system, 0, sizeof(buddy_system));

    for (i = 0; i < MAX_ORDER; ++i) {
        INIT_LIST_HEAD(&(buddy_system.free_area[i].free_list));
    }

    // init page map
    unsigned long page_map_size = pfn_cnt * sizeof(page_t);
    void *alloc_from_bootmem(unsigned long size, char *title);
    buddy_system.page_map = alloc_from_bootmem(page_map_size, "buddy");
    if (0 == buddy_system.page_map) {
        printk("can not go on playing...\n");
        while (1);
    }

    buddy_system.page_map_end = buddy_system.page_map + pfn_cnt + 1;
    printk("page_map begin %08x end %08x pfncnt %u page_t size %u\n", buddy_system.page_map, buddy_system.page_map_end,
           pfn_cnt, sizeof(page_t));
    for (i = 0; i < pfn_cnt; ++i) {
        page = buddy_system.page_map + i;
        memset((void *)page, 0, sizeof(page_t));
        page->private = 0;
        page->index = i;
        INIT_LIST_HEAD(&(page->lru));
        ClearPagePrivate(page);
    }

    // get free pages from bootmem
    for (i = 0; i < pfn_cnt; ++i) {
        page = buddy_system.page_map + i;

        if (BOOTMEM_PAGE_FREE == bootmem_page_state(i)) {
            // free to buddy system
            __free_pages(page, 0);
        }
    }

    // free bootmem bitmap pages to buddy system
    // ...

    dump_buddy_system();
}

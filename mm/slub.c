/*
 * ------------------------------------------------------------------------
 *   File Name: slub.c
 *      Author: Zhao Yanbai
 *              Wed Apr  9 16:51:07 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <irq.h>
#include <mm.h>
#include <slub.h>
#include <string.h>
#include <system.h>

page_t *get_partial(kmem_cache_t *cache, gfp_t gfpflags) {
    if (list_empty(&cache->partial)) return 0;

    list_head_t *p = cache->partial.next;
    list_del(p);

    page_t *page = 0;

    page = list_entry(p, page_t, lru);

    return page;
}

// 从伙伴系统批发页，并将之初始化成一个链表
page_t *new_slub(kmem_cache_t *cache, gfp_t gfpflags) {
    page_t *page = alloc_pages(gfpflags, cache->order);
    if (0 == page) {
        return 0;
    }

    unsigned long bgn = (unsigned long)page2va(page);
    unsigned long end = bgn + cache->objects * cache->size;

    unsigned long last = bgn;
    unsigned long addr;

    // 第一遍会将 bgn[0]的地址赋值给bgn[0]，也就是 bgn[0] = bgn + 0
    // 第二遍开始就是 bgn[n-1] = bgn + n
    for (addr = bgn; addr < end; addr += cache->size) {
        *((void **)last) = (void *)addr;
        last = addr;
    }

    // 最后一个赋值为0
    *((void **)last) = 0;

    page->freelist = (void **)bgn;
    page->inuse = 0;
    page->cache = cache;

    return page;
}

void *__slub_alloc(kmem_cache_t *cache, gfp_t gfpflags) {
    void **object = 0;
    page_t *page = 0;

    if (cache->page == 0) {
        page = get_partial(cache, gfpflags);
        // 如果partial为空，则上伙伴系统申请
        // 否则直接从partial上直接拿一个页来用
        if (page == 0) {
            cache->page = new_slub(cache, gfpflags);
        } else {
            cache->page = page;
        }
    }

    // 从partial和伙伴系统里没申请到页
    if (cache->page == 0) {
        return 0;
    }

    object = cache->page->freelist;

    assert(0 != object);

    cache->page->freelist = object[0];
    cache->page->inuse++;

    return object;
}

void *slub_alloc(kmem_cache_t *cache, gfp_t gfpflags) {
    void **object = 0;

    if (cache == 0) {
        return 0;
    }

    unsigned long flags;
    irq_save(flags);

    if (cache->page == 0 || cache->page->freelist == 0) {
        // 如果cache还没上buddy system批发页
        // 或者批发的页已经分配完了
        // 则需要换新的页: 1. 如果partial里有，就用partial的；2. 如果partial为空则上buddy system批发
        cache->page = 0;
        object = __slub_alloc(cache, gfpflags);
    } else {
        // 否则分配一个
        object = cache->page->freelist;
        cache->page->freelist = object[0];
        cache->page->inuse++;
    }

    irq_restore(flags);

    return object;
}

void __slub_free(kmem_cache_t *cache, page_t *page, void *addr) {
    void *prior;
    void **object = addr;

    prior = object[0] = page->freelist;
    page->freelist = object;

    if (page->inuse == 0) {
        list_del(&page->lru);
        free_pages((unsigned long)page2va(page));
    } else {
        if (prior == 0) {
            list_add(&page->lru, &cache->partial);
        }
    }
}

void slub_free(kmem_cache_t *cache, page_t *page, void *addr) {
    unsigned long flags;
    irq_save(flags);

    void **object = addr;

    page->inuse--;

    if (page == cache->page) {
        object[0] = page->freelist;
        page->freelist = object;
    } else {
        __slub_free(cache, page, addr);
    }

    irq_restore(flags);
}

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

page_t *new_slub(kmem_cache_t *cache, gfp_t gfpflags) {
    // alloc pages from buddy system
    unsigned long bgn = alloc_pages(gfpflags, cache->order);
    unsigned long end = 0;
    page_t *page = va2page(bgn);

    if (0 == page) return 0;

    end = bgn + cache->objects * cache->size;

    unsigned long last = bgn;
    unsigned long addr;
    for (addr = bgn; addr < end; addr += cache->size) {
        *((void **)last) = (void *)addr;
        last = addr;
    }

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
        if (page == 0) {
            page = new_slub(cache, gfpflags);
            if (page != 0) {
                cache->page = page;
            }
        } else {
            cache->page = page;
        }
    }

    if (cache->page == 0) {
        return 0;
    }

    object = cache->page->freelist;

    if (object == 0) {
        cache->page = 0;
    } else {
        cache->page->freelist = object[0];
        cache->page->inuse++;
    }

    return object;
}

void *slub_alloc(kmem_cache_t *cache, gfp_t gfpflags) {
    void **object = 0;

    if (cache == 0) return 0;

    unsigned long flags;
    irq_save(flags);

    if (cache->page == 0 || cache->page->freelist == 0) {
        cache->page = 0;
        object = __slub_alloc(cache, gfpflags);
    } else {
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
    }

    if (prior == 0) {
        list_add(&page->lru, &cache->partial);
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

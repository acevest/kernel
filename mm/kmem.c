/*
 * ------------------------------------------------------------------------
 *   File Name: kmem.c
 *      Author: Zhao Yanbai
 *              2021-11-13 12:21:11 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <irq.h>
#include <mm.h>
#include <slub.h>
#include <string.h>
#include <system.h>

static list_head_t slub_caches = LIST_HEAD_INIT(slub_caches);
static kmem_cache_t kmalloc_caches[SLUB_INIT_CACHE_SIZE];

bool calculate_params(kmem_cache_t* cache) {
    // calculate size
    unsigned long size = cache->objsize;
    unsigned long align = cache->align;

    align = KMALLOC_MIN_ALIGN > align ? KMALLOC_MIN_ALIGN : align;

    size = ALIGN(size, align);
    size = ALIGN(size, sizeof(void*));

    cache->size = size;
    cache->align = align;

    // 允许从buddy system最小分配一页
    unsigned long order = ~0UL;
    for (order = 0; order < MAX_ORDER; ++order) {
        if ((PAGE_SIZE << order) / cache->size >= 4) {
            cache->order = order;
            break;
        }
    }

    // 如果没有找到合适的从buddy system分配的大小则失败
    if (~0UL == cache->order) {
        panic("can not find a valid order\n");
        return false;
    }

    cache->objects = (PAGE_SIZE << cache->order) / cache->size;

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

bool kmem_cache_init(kmem_cache_t* cache, const char* name, size_t size, size_t align) {
    memset(cache, 0, sizeof(kmem_cache_t));

    cache->name = name;
    cache->objsize = size;
    cache->align = align;
    cache->page = 0;
    cache->partial_cnt = 0;
    INIT_LIST_HEAD(&(cache->partial));

    if (!calculate_params(cache)) {
        goto err;
    }

    return true;
err:
    panic("kmem_cache_init can not create cache\n");
    return false;
}

void* kmem_cache_alloc(kmem_cache_t* cache, gfp_t gfpflags) {
    return slub_alloc(cache, gfpflags);
}

void* kmem_cache_zalloc(kmem_cache_t* cache, gfp_t gfpflags) {
    void* p = slub_alloc(cache, gfpflags);
    if (0 != p) {
        memset(p, 0, cache->objsize);
    }
    return p;
}

void kmem_cache_free(kmem_cache_t* cache, void* addr) {
    page_t* page = 0;

    page = get_head_page(va2page((unsigned long)addr));

    slub_free(cache, page, addr);
}

kmem_cache_t* kmem_cache_create(const char* name, size_t size, size_t align) {
    kmem_cache_t* cache = kmalloc(sizeof(kmem_cache_t), 0);
    if (cache == 0) {
        return 0;
    }

    kmem_cache_init(cache, name, size, align);

    unsigned long flags;
    irq_save(flags);

    list_add(&(cache->list), &slub_caches);

    irq_restore(flags);

    return cache;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void* kmalloc(size_t size, gfp_t gfpflags) {
    unsigned int i;
    kmem_cache_t* cache = 0;
    void* addr = 0;

    for (i = 0; i < SLUB_INIT_CACHE_SIZE; ++i) {
        kmem_cache_t* p = kmalloc_caches + i;
        if (p->objsize >= size) {
            cache = p;
            break;
        }
    }

    if (0 != cache) {
        addr = kmem_cache_alloc(cache, gfpflags);
    }

    return addr;
}

void* kzalloc(size_t size, gfp_t gfpflags) {
    void* p = kmalloc(size, gfpflags);
    if (0 != p) {
        memset(p, 0, size);
    }

    return p;
}

void kfree(void* addr) {
    page_t* page = get_head_page(va2page((unsigned long)addr));
    kmem_cache_t* cache = page->cache;

    slub_free(cache, page, addr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////

void init_kmem_caches() {
    unsigned int i;
    kmem_cache_t* cache;

    for (i = SLUB_MIN_SHIFT; i < SLUB_MAX_SHIFT; ++i) {
        cache = kmalloc_caches + i - SLUB_MIN_SHIFT;

        kmem_cache_init(cache, "kmalloc", 1UL << i, KMALLOC_MIN_ALIGN);

        list_add(&(cache->list), &slub_caches);
    }
}

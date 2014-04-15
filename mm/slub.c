/*
 * ------------------------------------------------------------------------
 *   File Name: slub.c
 *      Author: Zhao Yanbai
 *              Wed Apr  9 16:51:07 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <mm.h>
#include <system.h>

list_head_t slub_caches;

typedef struct kmem_cache
{
    const char *name;

    unsigned long objsize;
    unsigned long size;
    unsigned long align;
    unsigned long order;
    unsigned long objects;

    unsigned int partial_cnt;
    list_head_t partial;

    page_t *page;

    void **freelist;
} kmem_cache_t;


#define SLUB_PAGE_SHIFT     PAGE_SHIFT
#define KMALLOC_MIN_SIZE    32
#define KMALLOC_MIN_ALIGN   32
static kmem_cache_t kmalloc_caches[SLUB_PAGE_SHIFT];

typedef unsigned int gfp_t;

static bool calculate_params(kmem_cache_t *cache)
{
    // calculate size
    unsigned long size = cache->objsize;
    unsigned long align= cache->align;
    align = KMALLOC_MIN_ALIGN > align ? KMALLOC_MIN_ALIGN : align;
    size = ALIGN(size, align);
    size = ALIGN(size, sizeof(void *));
    cache->size = size;

    // calculate order
    unsigned long order;
    for(order=1; order<MAX_ORDER; ++order)
    {
        if((PAGE_SIZE<<order) / cache->size >= 4)
        {
            cache->order = order;
            break;
        }
    }

    if(0 == cache->order)
    {
        printk("can not find a valid order\n");
        return false;
    }


    cache->objects = (PAGE_SIZE << cache->order) / cache->size;

    if(0 == cache->objects)
        return false;

    return true;
}

kmem_cache_t *kmem_cache_create(const char *name,
                                size_t size,
                                size_t align)
{


    return 0;
}


static bool kmem_cache_init(kmem_cache_t *cache,
                            const char *name,
                            size_t size,
                            size_t align)
{

    memset(cache, 0, sizeof(kmem_cache_t));

    cache->name         = name;
    cache->objsize      = size;
    cache->align        = align;
    cache->page         = 0;
    cache->freelist     = 0;
    cache->partial_cnt  = 0;
    INIT_LIST_HEAD(&(cache->partial));

    if(!calculate_params(cache))
        goto err;

    return true;

err:
    panic("kmem_cache_init can not create cache\n");
    return false;
}

void init_slub_system()
{
    unsigned int i;
    kmem_cache_t *cache;

    for(i=5; i<SLUB_PAGE_SHIFT; ++i)
    {
        cache = kmalloc_caches + i;
        kmem_cache_init(cache, "kmalloc", 1UL<<i, KMALLOC_MIN_ALIGN);
    }
}

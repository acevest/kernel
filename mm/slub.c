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

list_head_t slub_caches = LIST_HEAD_INIT(slub_caches);

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

//    void **freelist;

    list_head_t list;
} kmem_cache_t;


#define SLUB_MIN_SHIFT          5
#define SLUB_MAX_SHIFT          12
#define SLUB_INIT_CACHE_SIZE    ((SLUB_MAX_SHIFT) - (SLUB_MIN_SHIFT))
#define KMALLOC_MIN_SIZE        (1UL<<(SLUB_MIN_SHIFT))
#define KMALLOC_MIN_ALIGN       (1UL<<(SLUB_MIN_SHIFT))

static kmem_cache_t kmalloc_caches[SLUB_INIT_CACHE_SIZE];

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
    //cache->freelist     = 0;
    cache->partial_cnt  = 0;
    INIT_LIST_HEAD(&(cache->partial));

    if(!calculate_params(cache))
        goto err;


    return true;
err:
    panic("kmem_cache_init can not create cache\n");
    return false;
}

static page_t *get_partial(kmem_cache_t *cache, gfp_t gfpflags)
{
    if(list_empty(&cache->partial))
        return 0;

    list_head_t *p = cache->partial.next;
    list_del(p);

    page_t *page = 0;

    page = list_entry(p, page_t, lru);

    return page;
}

static page_t *new_slub(kmem_cache_t *cache, gfp_t gfpflags)
{
    // alloc pages from buddy system
    unsigned long bgn = alloc_pages(gfpflags, cache->order);
    unsigned long end = 0;
    page_t *page = va2page(bgn);

    if(0 == page)
        return 0;


    end = bgn + cache->objects*cache->size;

    unsigned long last = bgn;
    unsigned long addr;
    for(addr=bgn; addr<end; addr+=cache->size)
    {
        *((void **)last) = (void *) addr;
        last = addr;
    }

    *((void **)last) = 0;

    page->freelist = (void **)bgn;
    page->inuse    = 0;

    return page;
}

static void *__slub_alloc(kmem_cache_t *cache, gfp_t gfpflags)
{
    void **object = 0;
    page_t *page = 0;

    if(cache->page == 0)
    {
        page = get_partial(cache, gfpflags);
        if(page == 0)
        {
            page = new_slub(cache, gfpflags);
            if(page != 0)
            {
                cache->page = page;
            }
        }
    }

    if(cache->page == 0)
        return 0;

    object = cache->page->freelist;

    if(object == 0)
    {
        cache->page = 0;
    }
    else
    {
        cache->page->freelist = object[0];
        cache->page->inuse++;
    }

    return object;
}

static void *slub_alloc(kmem_cache_t *cache, gfp_t gfpflags)
{
    void **object = 0;

    unsigned long objsize = cache->objsize;

    if(cache->page == 0 || cache->page->freelist == 0)
    {
        cache->page = 0;
        object = __slub_alloc(cache, gfpflags);
    }
    else
    {
        object = cache->page->freelist;
        cache->page->freelist = object[0];
    }

    return object;
}

static void __slub_free(kmem_cache_t *cache, page_t *page, void *addr)
{
    void *prior;
    void **object = addr;

    prior = object[0] = page->freelist;
    page->freelist = object;
    page->inuse--;

    if(page->inuse == 0)
    {
        list_del(&page->lru);
        free_pages((unsigned long)page2va(page));
    }

    if(prior == 0)
    {
        list_add(&page->lru, &cache->partial);
    }
}

static void slub_free(kmem_cache_t *cache, page_t *page, void *addr)
{
    void **object = addr;

    if(page == cache->page)
    {
        object[0] = page->freelist;
        page->freelist = object;
    }
    else
    {
        __slub_free(cache, page, addr);
    }
}

void kmem_cache_free(kmem_cache_t *cache, void *addr)
{
    page_t *page = 0;

    page = get_head_page(va2page((unsigned long)addr));

    slub_free(cache, page, addr);
}


void init_slub_system()
{
    unsigned int i;
    kmem_cache_t *cache;

    for(i=SLUB_MIN_SHIFT; i<SLUB_MAX_SHIFT; ++i)
    {
        cache = kmalloc_caches + i - SLUB_MIN_SHIFT;
        kmem_cache_init(cache, "kmalloc", 1UL<<i, KMALLOC_MIN_ALIGN);

        //cache->inx = i;

        list_add(&(cache->list), &slub_caches);
    }

    list_head_t *p;
    list_for_each(p, &slub_caches)
    {
        cache = list_entry(p, kmem_cache_t, list);
        printk("cache size %d align %d \n", cache->size, cache->align);
    }

    for(i=0; i<10; ++i)
    {
        void *addr = slub_alloc(kmalloc_caches+2, 0);
        printk("slub addr %08x\n", (unsigned long)addr);
    }
}

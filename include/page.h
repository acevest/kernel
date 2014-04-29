/*
 *--------------------------------------------------------------------------
 *   File Name: page.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Sat Feb  7 21:47:42 2009
 * Last Update: Sat Feb  7 21:47:42 2009
 * 
 *--------------------------------------------------------------------------
 */

#ifndef _PAGE_H
#define _PAGE_H



#define PAGE_P      0x1
#define PAGE_WR     0x2
#define PAGE_US     0x4

#define PAGE_SHIFT      (12)
#define PAGE_SIZE       (1UL << PAGE_SHIFT)
#define PAGE_MASK       (~((1UL << PAGE_SHIFT)-1))
#define PAGE_OFFSET     (0xC0000000)
#define PAGE_PDE_CNT    1024
#define PAGE_PTE_CNT    1024

#ifndef ASM
#include <types.h>
#include <bits.h>
#define get_npd(addr)    (((u32)(addr))>>22)
#define get_npt(addr)    ((((u32)(addr))>>12) & 0x3FF)

#include <list.h>

typedef unsigned long pde_t;
typedef unsigned long pte_t;

#define PDECNT_PER_PAGE (PAGE_SIZE/sizeof(pde_t))
#define PTECNT_PER_PAGE (PAGE_SIZE/sizeof(pte_t))

#define PAGE_ITEMS (PAGE_SIZE/sizeof(unsigned long))
#define PAGE_ALIGN(page)    (page & PAGE_MASK)
#define PAGE_UP(page)     (((unsigned long)page + PAGE_SIZE -1) & PAGE_MASK)
#define PAGE_DOWN    PAGE_ALIGN

#define va2pa(x) (((unsigned long)(x)) - PAGE_OFFSET)
#define pa2va(x) ((void *) (((unsigned long)(x)) + PAGE_OFFSET))

// pfn: page frame number
#define pa2pfn(addr)    ((addr)>>PAGE_SHIFT)
#define pfn2pa(pfn)     ((pfn)<<PAGE_SHIFT)

#define va2pfn(addr)    pa2pfn(va2pa(addr))
#define pfn2va(pfn)     pa2va(pfn2pa(pfn))

#define valid_va(addr)  ((addr) >= PAGE_OFFSET)

#define PFN_UP(addr)    (((addr) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DW(addr)    ((addr) >> PAGE_SHIFT)

#define MAX_ORDER       (5)
#define MAX_OLD_ORDER   (11)

#define LOAD_CR3(pde)   asm("movl %%edx, %%cr3"::"d"(va2pa(pde)))


typedef unsigned int gfp_t;

enum page_flags {
    PG_Private,
};
 
struct kmem_cache;
typedef struct kmem_cache kmem_cache_t;

typedef struct page
{
    unsigned long flags;
    unsigned long private;
    unsigned long index;
    list_head_t   lru;

    struct page   *head_page;
    unsigned int order;

    void **freelist;    // for slub
    kmem_cache_t *cache;

    unsigned long inuse;
} page_t;

void *page2va(page_t *page);
page_t *va2page(unsigned long addr);

static inline page_t *get_head_page(page_t *page) { return page->head_page; }

#define __GETPAGEFLAG(name)                                         \
    static inline int Page##name(page_t *page)                      \
            {return constant_test_bit(PG_##name, &page->flags); }

#define __SETPAGEFLAG(name)                                         \
    static inline int SetPage##name(page_t *page)                   \
            {return test_and_set_bit(PG_##name, &page->flags); }

#define __CLEARPAGEFLAG(name)                                       \
    static inline int ClearPage##name(page_t *page)                 \
            {return test_and_clear_bit(PG_##name, &page->flags); }


__GETPAGEFLAG(Private)
__SETPAGEFLAG(Private)
__CLEARPAGEFLAG(Private)


typedef struct free_area
{
    unsigned long free_count;
    list_head_t   free_list;
} free_area_t;


unsigned long alloc_pages(unsigned int gfp_mask, unsigned int order);
void free_pages(unsigned long addr);


struct kmem_cache
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

    list_head_t list;
};


// TODO Remove
typedef struct page_
{
    //struct page *prev, *next;
    ListHead list;
    unsigned int order;
    unsigned int mapNR;
    unsigned int count;
} Page, *pPage;

typedef struct free_area_
{
    //struct page *prev, *next;
    ListHead freeList;
    unsigned char *map;
    unsigned int mapSize;
    unsigned int count;
} FreeArea, *pFreeArea;


pPage   old_alloc_pages(unsigned int order);
void    old_free_pages(pPage page);
//void    free_pages(pPage page, unsigned int order);
void    disp_free_area();


#endif    // ASM

#endif //_PAGE_H

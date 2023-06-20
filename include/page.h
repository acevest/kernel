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

/*

| CR0的PG | CR4的PAE | CR4的PSE | PDE的PS | 页面规模 | 物理地址规模 |
| ------- | -------- | -------- | ------- | -------- | ------------ |
| 0       | ×       | ×       | ×      | ―      | 禁止分页 |
| 1       | 0        | 0        | ×      | 4KB      | 32位        |
| 1       | 0        | 1        | 0       | 4KB      | 32位        |
| 1       | 0        | 1        | 1       | 4MB      | 32位        |
| 1       | 1        | ×       | 0       | 4KB      | 36位        |
| 1       | 1        | ×       | 1       | 2MB      | 36位        |

*/

#define PAGE_P 0x1   // 在内存中
#define PAGE_WR 0x2  // 表示可读写
#define PAGE_US 0x4  // 用户级

#define PAGE_SHIFT (12)
#define PAGE_SIZE (1UL << PAGE_SHIFT)
#define PAGE_FLAG_MASK ((1UL << PAGE_SHIFT) - 1)
#define PAGE_MASK (~PAGE_FLAG_MASK)
#define PAGE_OFFSET (0xC0000000)
#define PAGE_PDE_CNT 1024
#define PAGE_PTE_CNT 1024

// P：  有效位。0 表示当前表项无效。
// R/W: 0 表示只读。1表示可读写。
// U/S: 0 表示只能0、1、2特权级可访问。3 表示只有特权级程序可访问
// A:   0 表示该页未被访问，1表示已被访问。
// D:   脏位。0表示该页未写过，1表示该页被写过
// PS:  只存在于页目录表。在CR4中的PSE打开的情况下，0表示这是4KB页，指向一个页表。1表示这是4MB大页，直接指向物理页。

// |<------ 31~12------>|<------ 11~0 --------->| 比特
//                      |b a 9 8 7 6 5 4 3 2 1 0|
// |--------------------|-|-|-|-|-|-|-|-|-|-|-|-| 占位
// |<-------index------>| AVL |G|P|0|A|P|P|U|R|P| 属性
//                              |S|   |C|W|/|/|
//                                    |D|T|S|W|

#define PDE_P 0x001
#define PDE_RW 0x002
#define PDE_US 0x004
#define PDE_PWT 0x008
#define PDE_PCD 0X010
#define PDE_A 0x020
// #define PDE_D 0x040
#define PDE_PS 0x080
#define PDE_G 0x100
#define PDE_AVL 0xE00

// |<------ 31~12------>|<------ 11~0 --------->| 比特
//                      |b a 9 8 7 6 5 4 3 2 1 0|
// |--------------------|-|-|-|-|-|-|-|-|-|-|-|-| 占位
// |<-------index------>| AVL |G|P|D|A|P|P|U|R|P| 属性
//                              |A|   |C|W|/|/|
//                              |T|   |D|T|S|W|
#define PTE_P 0x001
#define PTE_RW 0x002
#define PTE_US 0x004
#define PTE_PWT 0x008
#define PTE_PCD 0X010
#define PTE_A 0x020
#define PTE_D 0x040
#define PTE_PAT 0x080
#define PTE_G 0x100
#define PTE_AVL 0xE00

#ifndef ASM
#include <bits.h>
#include <types.h>
#define get_npde(addr) (((u32)(addr)) >> 22)
#define get_npte(addr) ((((u32)(addr)) >> 12) & 0x3FF)

#include <list.h>

typedef unsigned long pde_t;
typedef unsigned long pte_t;

#define PDECNT_PER_PAGE (PAGE_SIZE / sizeof(pde_t))
#define PTECNT_PER_PAGE (PAGE_SIZE / sizeof(pte_t))

#define PAGE_ITEMS (PAGE_SIZE / sizeof(unsigned long))
#define PAGE_ALIGN(page) (((unsigned long)(page)) & PAGE_MASK)
#define PAGE_UP(page) (((unsigned long)page + PAGE_SIZE - 1) & PAGE_MASK)
#define PAGE_DOWN PAGE_ALIGN

#define PAGE_FLAGS(addr) ((addr)-PAGE_ALIGN(addr))

#define va2pa(x) (((unsigned long)(x)) - PAGE_OFFSET)
#define pa2va(x) ((void *)(((unsigned long)(x)) + PAGE_OFFSET))

// pfn: page frame number
#define pa2pfn(addr) ((addr) >> PAGE_SHIFT)
#define pfn2pa(pfn) ((pfn) << PAGE_SHIFT)

#define va2pfn(addr) pa2pfn(va2pa(addr))
#define pfn2va(pfn) pa2va(pfn2pa(pfn))

#define valid_va(addr) ((addr) >= PAGE_OFFSET)

#define PFN_UP(addr) (((addr) + PAGE_SIZE - 1) >> PAGE_SHIFT)
#define PFN_DW(addr) ((addr) >> PAGE_SHIFT)

#define MAX_ORDER (11)

#define LoadCR3(cr3) asm volatile("movl %%edx, %%cr3" ::"d"(cr3))

typedef unsigned int gfp_t;

enum page_flags {
    PG_Private,
};

struct kmem_cache;
typedef struct kmem_cache kmem_cache_t;

struct blk_buffer;
typedef struct page {
    unsigned long count;
    unsigned long flags;
    unsigned long private;
    unsigned long index;
    list_head_t lru;

    struct page *head_page;  // buddy system
    unsigned int order;

    void **freelist;  // for slub

    kmem_cache_t *cache;

    unsigned long inuse;

    //
    struct blk_buffer *buffers;
} page_t;

void *page2va(page_t *page);
page_t *_va2page(unsigned long addr);
page_t *_pa2page(unsigned long addr);

#define va2page(addr) _va2page(PAGE_ALIGN(addr))
#define pa2page(addr) _pa2page(PAGE_ALIGN(addr))

static inline page_t *get_head_page(page_t *page) { return page->head_page; }

#define __GETPAGEFLAG(name) \
    static inline int Page##name(page_t *page) { return constant_test_bit(PG_##name, &page->flags); }

#define __SETPAGEFLAG(name) \
    static inline int SetPage##name(page_t *page) { return test_and_set_bit(PG_##name, &page->flags); }

#define __CLEARPAGEFLAG(name) \
    static inline int ClearPage##name(page_t *page) { return test_and_clear_bit(PG_##name, &page->flags); }

__GETPAGEFLAG(Private)
__SETPAGEFLAG(Private)
__CLEARPAGEFLAG(Private)

typedef struct free_area {
    unsigned long free_count;
    list_head_t free_list;
} free_area_t;

unsigned long alloc_pages(unsigned int gfp_mask, unsigned int order);
void free_pages(unsigned long addr);

#define alloc_one_page(gfp_mask) alloc_pages(gfp_mask, 0)

struct kmem_cache {
    const char *name;

    // 预期分配的大小
    unsigned long objsize;

    // 实际分配的大小
    unsigned long size;

    // 对齐
    unsigned long align;

    // 从buddy system批发的页的幂数
    unsigned long order;

    //
    unsigned long objects;

    // 没有完全分配的页的链表及数量
    unsigned int partial_cnt;
    list_head_t partial;

    // 正在分配的页
    page_t *page;

    list_head_t list;
};

#endif  // ASM

#endif  //_PAGE_H

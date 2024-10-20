/*
 * ------------------------------------------------------------------------
 *   File Name: mm.h
 *      Author: Zhao Yanbai
 *              Sun Mar 30 11:10:00 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <page.h>

#define BOOTMEM_PAGE_FREE 0
#define BOOTMEM_PAGE_USED 1

void *alloc_bootmem(unsigned long size, unsigned long align);
unsigned long bootmem_max_pfn();
unsigned long bootmem_page_state(unsigned long pfn);

kmem_cache_t *kmem_cache_create(const char *name, size_t size, size_t align);
void *kmem_cache_alloc(kmem_cache_t *cache, gfp_t gfpflags);
void *kmem_cache_zalloc(kmem_cache_t *cache, gfp_t gfpflags);

#define VM_READ 0x00000001
#define VM_WRITE 0x00000002
#define VM_EXEC 0x00000004
#define VM_GROW_UP 0x10000000
#define VM_GROW_DOWN 0x20000000

typedef struct vm_area {
    uint32_t vm_bgn;
    uint32_t vm_end;

    uint32_t vm_flags;

    struct vma *vm_next;
} vm_area_t;

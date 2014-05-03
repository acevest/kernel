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
unsigned long bootmem_total_pages();
unsigned long bootmem_page_state(unsigned long pfn);

#define bootmem_alloc_pages(n) alloc_bootmem((n)*PAGE_SIZE, PAGE_SIZE)


kmem_cache_t *kmem_cache_create(const char *name, size_t size, size_t align);
void *kmem_cache_alloc(kmem_cache_t *cache, gfp_t gfpflags);

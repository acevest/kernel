/*
 * ------------------------------------------------------------------------
 *   File Name: slub.h
 *      Author: Zhao Yanbai
 *              2021-11-13 12:22:22 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#define SLUB_MIN_SHIFT 4
#define SLUB_MAX_SHIFT 16
#define SLUB_INIT_CACHE_SIZE ((SLUB_MAX_SHIFT) - (SLUB_MIN_SHIFT))
#define KMALLOC_MIN_SIZE (1UL << (SLUB_MIN_SHIFT))
#define KMALLOC_MIN_ALIGN (1UL << (SLUB_MIN_SHIFT))

void* slub_alloc(kmem_cache_t* cache, gfp_t gfpflags);
void slub_free(kmem_cache_t* cache, page_t* page, void* addr);

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

void *alloc_bootmem(unsigned long size, unsigned long align);

#define bootmem_alloc_pages(n) alloc_bootmem((n)*PAGE_SIZE, PAGE_SIZE)


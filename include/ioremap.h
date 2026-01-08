/*
 * ------------------------------------------------------------------------
 *   File Name: ioremap.h
 *      Author: Zhao Yanbai
 *              2026-01-02 13:38:08 Friday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <vmalloc.h>

void* ioremap(paddr_t paddr, size_t size);
void iounmap(vaddr_t vaddr);

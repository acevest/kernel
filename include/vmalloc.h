/*
 * ------------------------------------------------------------------------
 *   File Name: vmalloc.h
 *      Author: Zhao Yanbai
 *              2026-01-02 12:21:21 Friday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <types.h>
#include <kdef.h>

#define VM_ALLOC 0x01
#define VM_IOREMAP 0x02

typedef struct vm_struct {
    vaddr_t vm_vaddr;
    size_t vm_size;
    paddr_t vm_paddr;
    uint32_t vm_flags;
    struct vm_struct* vm_next;
} vm_struct_t;

vm_struct_t* get_vm_area(size_t size, uint32_t flags);
vm_struct_t* find_vm_area(vaddr_t vaddr);

void free_vm_area(vm_struct_t* vm);

/*
 * ------------------------------------------------------------------------
 *   File Name: ioremap.c
 *      Author: Zhao Yanbai
 *              2026-01-02 13:38:43 Friday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <ioremap.h>
#include <page.h>
#include <assert.h>
#include <printk.h>

void ioremap_page_range(vaddr_t vaddr, paddr_t paddr, size_t size) {
    assert(PAGE_DOWN(size) == size);

    for (int i = 0; i < size / PAGE_SIZE; i++) {
        page_map(vaddr + i * PAGE_SIZE, paddr + i * PAGE_SIZE, PAGE_P | PAGE_WR);
    }
}

void* ioremap(paddr_t phys_addr, size_t size) {
    size = PAGE_UP(size);

    vm_struct_t* vm_area = get_vm_area(size, VM_IOREMAP);
    if (!vm_area) {
        return NULL;
    }

    // 映射物理地址到虚拟地址
    vm_area->vm_paddr = phys_addr;
    vm_area->vm_flags |= VM_IOREMAP;

    printk("ioremap: 0x%08x -> 0x%08x size %u\n", phys_addr, vm_area->vm_vaddr, vm_area->vm_size);

    ioremap_page_range(vm_area->vm_vaddr, vm_area->vm_paddr, vm_area->vm_size);

    return (void*)vm_area->vm_vaddr;
}

void iounmap(void* addr) {
    // vm_struct_t* vm_area = find_vm_area((vaddr_t)addr);
    // if (!vm_area) {
    //     return;
    // }

    // if (!(vm_area->vm_flags & VM_IOREMAP)) {
    //     return;
    // }

    // free_vm_area(vm_area);
}

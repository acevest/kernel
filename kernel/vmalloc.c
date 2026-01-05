/*
 * ------------------------------------------------------------------------
 *   File Name: vmalloc.c
 *      Author: Zhao Yanbai
 *              2026-01-02 12:21:28 Friday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <vmalloc.h>
#include <assert.h>
#include <irq.h>
#include <page.h>
#include <assert.h>

static vm_struct_t* vm_area_list = NULL;

vaddr_t vm_vaddr_base = VMALLOC_VADDR_BASE + (8 * 1024 * 1024);
vaddr_t vm_vaddr_end = VMALLOC_VADDR_END;

vm_struct_t* get_vm_area(size_t size, uint32_t flags) {
    // assert size是页对齐
    assert(PAGE_DOWN(size) == size);
    assert(vm_vaddr_base < vm_vaddr_end);

    size += PAGE_SIZE;  // 尾部添加一个空页

    vaddr_t vm_area_gap_bgn = 0;
    vaddr_t vm_area_gap_end = 0;

    uint32_t eflags;
    irq_save(eflags);

    vm_struct_t* prev = NULL;
    vm_struct_t* curt = vm_area_list;

    while (curt != NULL) {
        assert(curt->vm_vaddr >= vm_vaddr_base);
        assert(curt->vm_vaddr + curt->vm_size <= vm_vaddr_end);

        //
        vm_area_gap_bgn = prev == NULL ? vm_vaddr_base : prev->vm_vaddr + prev->vm_size;
        vm_area_gap_end = curt->vm_vaddr;

        assert(vm_area_gap_bgn <= vm_area_gap_end);

        // 找到了
        if (vm_area_gap_end - vm_area_gap_bgn >= size) {
            break;
        }

        prev = curt;
        curt = curt->vm_next;
    }

    // 如果找到最后了也还没找到
    if (curt == NULL) {
        vm_area_gap_bgn = prev == NULL ? vm_vaddr_base : prev->vm_vaddr + prev->vm_size;
        vm_area_gap_end = vm_vaddr_end;

        assert(vm_area_gap_bgn <= vm_area_gap_end);

        if (vm_area_gap_end - vm_area_gap_bgn < size) {
            irq_restore(eflags);
            return NULL;
        }
    }

    // 创建一人vm_struct
    vm_struct_t* vm_area = kmalloc(sizeof(vm_struct_t), 0);
    if (vm_area == NULL) {
        irq_restore(eflags);
        return NULL;
    }

    vm_area->vm_vaddr = vm_area_gap_bgn;
    vm_area->vm_size = size;
    vm_area->vm_flags = flags;

    // add to list
    vm_area->vm_next = curt;
    if (prev == NULL) {
        vm_area_list = vm_area;
    } else {
        prev->vm_next = vm_area;
    }

    irq_restore(eflags);

    return vm_area;
}

vm_struct_t* find_vm_area(vaddr_t vaddr) {
    assert(PAGE_DOWN(vaddr) == vaddr);
    assert(vaddr >= vm_vaddr_base);
    assert(vaddr < vm_vaddr_end);
    assert(vm_area_list != NULL);

    vm_struct_t* curt = vm_area_list;

    while (curt != NULL) {
        assert(curt->vm_size != 0);
        assert(curt->vm_vaddr >= vm_vaddr_base);
        assert(curt->vm_vaddr + curt->vm_size <= vm_vaddr_end);

        if (vaddr == curt->vm_vaddr) {
            return curt;
        }

        assert(vaddr >= curt->vm_vaddr + curt->vm_size);

        curt = curt->vm_next;
    }

    return NULL;
}

void free_vm_area(vm_struct_t* vm_area) {
    assert(vm_area != NULL);
    assert(vm_area->vm_size != 0);
    assert(vm_area->vm_vaddr >= vm_vaddr_base);
    assert(vm_area->vm_vaddr + vm_area->vm_size <= vm_vaddr_end);
    assert(vm_area_list != NULL);
    // TODO
}

void* vmalloc(size_t size) {
    return NULL;
}

void vfree(void* addr) {
}

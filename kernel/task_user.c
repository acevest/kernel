/*
 * ------------------------------------------------------------------------
 *   File Name: task_user.c
 *      Author: Zhao Yanbai
 *              2021-11-15 12:21:31 Monday CST
 * Description: none
 * ------------------------------------------------------------------------
 */
#include <fcntl.h>
#include <io.h>
#include <irq.h>
#include <page.h>
#include <processor.h>
#include <sched.h>
#include <stat.h>
#include <stdio.h>
#include <syscall.h>
#include <system.h>
#include <types.h>
#include <page.h>

void flush_tlb() {
    asm volatile(
        "movl %%cr3, %%eax;"
        "movl %%eax, %%cr3;"
        :
        :
        : "eax");
}

void user_task_entry() {
    current->priority = 79;

    // ring3只占用一个page，页的起始位置放的是代码，页的末尾当栈用
    // ring3的地址直接是物理地址
    extern uint8_t ring3_page_begin;

    paddr_t ring3_page_addr = (paddr_t)&ring3_page_begin;  // 不在内核空间的物理地址
    // paddr_t ring3_stack_top = ring3_page_addr + PAGE_SIZE;

    vaddr_t ring3_page_vaddr = 0x08000000;  // 指定的ring3的虚拟地址
    vaddr_t ring3_stack_top_vaddr = PAGE_OFFSET - 0x100000;

    page_map(ring3_page_vaddr, ring3_page_addr, PAGE_P | PAGE_US);

    // 这里减去PAGE_SIZE是因为栈是向下生长的，所以得把栈顶虚拟地址空间的前一页映射到ring3_page
    page_map((ring3_stack_top_vaddr - PAGE_SIZE), ring3_page_addr, PAGE_P | PAGE_US | PAGE_WR);

    {
        // 在ring3_page里非代码的地方插入特征值方便调试
        extern uint8_t ring3_text_end;
        uint32_t ring3_text_size = (&ring3_text_end) - (&ring3_page_begin);
        for (uint32_t i = ring3_text_size; i < PAGE_SIZE; i++) {
            ((uint8_t*)(pa2va(ring3_page_addr)))[i] = 0xCC;
        }
    }

    asm volatile("sysexit;" ::"d"(ring3_page_vaddr), "c"(ring3_stack_top_vaddr));
}

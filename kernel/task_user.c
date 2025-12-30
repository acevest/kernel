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

#define RING3_ENTRY __attribute__((__section__(".ring3.entry")))
void RING3_ENTRY __attribute__((__aligned__(PAGE_SIZE))) ring3_entry() {
    while (1) {
        int __sysc_ret__ = 0;

        // EDI = 用户态返回地址
        // EBP = 用户态栈
        asm volatile(
            "nop;"
            "nop;"
            "pusha;"
            "call get_eip;"
            "get_eip:"
            "popl %%edi;" // 获得EIP的值，注意此时EIP的位置为这条pop指令的地址
                          // 反汇编后计算了一下需要加8个字节才能跳到sysenter下一个指令
            "addl $8, %%edi;"
            "movl %%esp, %%ebp;"
            "sysenter;"
            "popa;"
            "nop;"
            "nop;"
            :"=a"(__sysc_ret__)
            :"a"(SYSC_WAIT)
        );
    }
}

void flush_tlb() {
    asm volatile("movl %%cr3, %%eax;"
                "movl %%eax, %%cr3;"
                :
                :
                : "eax");
}

void user_task_entry() {
    current->priority = 7;

    // ring3只占用一个page，页的起始位置放的是代码，页的末尾当栈用
    // ring3的地址直接是物理地址
    extern unsigned long ring3_page_addr;
    extern unsigned long ring3_page_end;
    unsigned long ring3_text_page =(unsigned long) &ring3_page_addr; // 不在内核空间的物理地址
    unsigned long ring3_stack_top = (unsigned long) &ring3_page_end;  // 不在内核空间的物理地址

    unsigned long ring3_page_vaddr = 0x08000000; // 指定的ring3的虚拟地址
    unsigned long ring3_stack_top_vaddr = ring3_page_vaddr + (ring3_stack_top - ring3_text_page);
    int npte = get_npte(ring3_page_vaddr);
    int npde = get_npde(ring3_page_vaddr);

    // 分配一个页表在用户空间对这页进行映射
    pte_t *pgt = (pte_t *)page2va(alloc_one_page(0));

    memset(pgt, 0, PAGE_SIZE);
    pgt[npte] = (pte_t)(ring3_text_page | PAGE_P | PAGE_US | PAGE_WR);

    // 把这个页表映射到页目录
    pde_t *pgd = pa2va(get_pgd());
    pgd[npde] = (pde_t)(((unsigned long)va2pa(pgt)) | PAGE_P | PAGE_US | PAGE_WR);

    // 到此完成了 ring3_page_vaddr -> ring3_text_page的映射
    // 刷新tlb
    flush_tlb();

    // 现在准备返回用户态
    asm volatile("sysexit;" ::"d"(ring3_page_vaddr), "c"(ring3_stack_top_vaddr - 0x10));
}

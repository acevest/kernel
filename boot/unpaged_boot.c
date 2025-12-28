/*
 * ------------------------------------------------------------------------
 *   File Name: unpaged_boot.c
 *      Author: Zhao Yanbai
 *              2025-12-27 21:21:24 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <types.h>
#include <msr.h>
#include <page.h>
#include <linkage.h>
#include <boot.h>



extern unsigned long kernel_virtual_addr_start;
extern pde_t __initdata init_pgd[PDECNT_PER_PAGE] __attribute__((__aligned__(PAGE_SIZE)));
extern pte_t __initdata init_pgt[PTECNT_PER_PAGE * BOOT_INIT_PAGETBL_CNT] __attribute__((__aligned__(PAGE_SIZE)));

// Length = BOOT_INIT_PAGETBL_CNT*4M
// [0x00000000, 0x00000000 + Length - 1]
// [0xC0000000, 0xC0000000 + Length - 1]
// 这两个线性地址空间都映射到同一片物理内存空间: [0x00000000, 0x00000000 + Length -1]
// 总共 BOOT_INIT_PAGETBL_CNT * 4M 物理内存
void boot_paging() {
    unsigned long init_pgd_paddr = va2pa(init_pgd);
    unsigned long init_pgt_paddr = va2pa(init_pgt);

    // 清空页目录
    pde_t *dir =(pde_t *)init_pgd_paddr;
    for(int i=0; i<PDECNT_PER_PAGE; i++) {
        dir[i] = 0;
    }

    // 初始化页目录
    unsigned long kpde_base = get_npde((unsigned long)(&kernel_virtual_addr_start));
    pde_t pd_entry = init_pgt_paddr | PAGE_US | PAGE_WR | PAGE_P;
    for(int i=0; i<BOOT_INIT_PAGETBL_CNT; i++) {
        dir[i] = pd_entry; // 设置低端线性地址空间的页表项
        dir[kpde_base+i] = pd_entry; // 设置内核线性地址空间的页表项
        pd_entry += PAGE_SIZE;
    }


    pte_t *table = (pte_t *)init_pgt_paddr;
    pte_t pt_entry = PAGE_US | PAGE_WR | PAGE_P;
    for(int i=0; i<BOOT_INIT_PAGETBL_CNT*PTECNT_PER_PAGE; i++) {
        table[i] = pt_entry;
        pt_entry += PAGE_SIZE;
    }


    // 设置页目录
    asm volatile("mov %0, %%cr3"::"r"(init_pgd_paddr));
}


void lapic_init() {
    // cpuid_regs_t r;
    // r = cpuid(1);
    // if(r.edx & (1 << 9)) {
    //     printk("local apic supported\n");
    //     if(r.ecx & (1 << 21)) {
    //         printk("x2apic supported\n");
    //     } else {
    //         panic("x2apic not supported\n");
    //     }
    // } else {
    //     panic("local apic not supported\n");
    // }

    uint64_t apic_base = read_msr(MSR_IA32_APIC_BASE);
    // printk("apic base: %016lx\n", apic_base);

    // 开启2xapic
    apic_base |= (1 << 10);
    write_msr(MSR_IA32_APIC_BASE, apic_base);

    apic_base = read_msr(MSR_IA32_APIC_BASE);
    // printk("after 2xapic enable apic base: %016lx\n", apic_base);
}

/*
 * ------------------------------------------------------------------------
 *   File Name: apic.c
 *      Author: Zhao Yanbai
 *              2025-12-28 20:52:47 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <msr.h>
#include <cpuid.h>
#include <system.h>

 void lapic_init() {
    cpuid_regs_t r;
    r = cpuid(1);
    if(r.edx & (1 << 9)) {
        printk("local apic supported\n");
        if(r.ecx & (1 << 21)) {
            printk("x2apic supported\n");
        } else {
            panic("x2apic not supported\n");
        }
    } else {
        panic("local apic not supported\n");
    }

    uint64_t apic_base = read_msr(MSR_IA32_APIC_BASE);
    printk("apic base: %016lx\n", apic_base);


    // apic 必然已经开启
    assert((apic_base & (1 << 11)) != 0);

    // 开启2xapic
    apic_base |= (1 << 10);
    write_msr(MSR_IA32_APIC_BASE, apic_base);

    apic_base = read_msr(MSR_IA32_APIC_BASE);
    printk("after 2xapic enable apic base: %016lx\n", apic_base);

    uint64_t apic_version = read_msr(MSR_IA32_X2APIC_VERSION);
    printk("apic version: %08lx\n", apic_version);


    unsigned long apic_phys_base_addr = apic_base & 0xFFFFF000;
    unsigned long apic_virt_base_addr = apic_phys_base_addr;
    #if 0
    unsigned long ddd = 0xFEC00000;
    while(ddd < 0xFF000000)  {
        page_map((void*)ddd, (void*)ddd, PAGE_P);
        ddd += 0x1000;
    }
    #endif
    page_map((void*)apic_virt_base_addr, (void*)apic_phys_base_addr, PAGE_P);

    {
        volatile uint32_t *base = (volatile uint32_t *)apic_virt_base_addr;
        uint32_t id = base[0x20/4]; // APIC ID 偏移
        uint32_t version = base[0x30/4];
        printk("APIC id %08x version %08x\n", id, version);
    }

}


void init_apic() {
#if 0
    lapic_init();
#endif
}

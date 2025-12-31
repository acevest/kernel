/*
 *--------------------------------------------------------------------------
 *   File Name: cpuid.c
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Sat Feb 28 14:43:35 2009
 * Last Update: Sat Feb 28 14:43:35 2009
 *
 *--------------------------------------------------------------------------
 */
#include <bits.h>
#include <printk.h>
#include <string.h>
#include <cpuid.h>

#define TEST_FEATURE(val, bit, fea) \
    do {                            \
        if (ISSET_BIT(val, bit))    \
            printk(" %s", fea);     \
    } while (0);

cpuid_regs_t cpuid(unsigned long op) {
    cpuid_regs_t r;
    asm("cpuid;" : "=a"(r.eax), "=b"(r.ebx), "=c"(r.ecx), "=d"(r.edx) : "a"(op));

    return r;
}

void detect_cpu() {
    cpuid_regs_t r;
    unsigned short int cpu_sn[6];  // serial number
    int i;

    /**********************Get CPU Name********************************/
    char cpu_name[13];

    r = cpuid(0);
    memcpy(cpu_name + 0, &r.ebx, 4);
    memcpy(cpu_name + 4, &r.edx, 4);
    memcpy(cpu_name + 8, &r.ecx, 4);
    cpu_name[12] = 0;
    printk("%s ", cpu_name);

    /**********************Get Processor Brand String******************/
    char pbs[50];  // processor brand string
    r = cpuid(0x80000002);
    memcpy(pbs + 0, &r.eax, 4);
    memcpy(pbs + 4, &r.ebx, 4);
    memcpy(pbs + 8, &r.ecx, 4);
    memcpy(pbs + 12, &r.edx, 4);
    r = cpuid(0x80000003);
    memcpy(pbs + 16, &r.eax, 4);
    memcpy(pbs + 20, &r.ebx, 4);
    memcpy(pbs + 24, &r.ecx, 4);
    memcpy(pbs + 28, &r.edx, 4);
    r = cpuid(0x80000004);
    memcpy(pbs + 32, &r.eax, 4);
    memcpy(pbs + 36, &r.ebx, 4);
    memcpy(pbs + 40, &r.ecx, 4);
    memcpy(pbs + 44, &r.edx, 4);
    pbs[48] = 0;
    printk("%s", pbs);

    /**********************Get Number of Processors********************/
    int pn;  // number of logical processors in one physical processor
    r = cpuid(1);
    pn = ((r.ebx & 0x00FF0000) >> 16);
    printk(" x %d Cores\n", pn);
    if (((r.ecx >> 21) & 0x01) == 1) {
        printk("x2APIC\n");
    }
    printk("ECX %x\n", r.ecx);
    printk("APIC ID: %x\n", (r.ebx >> 24) & 0xFF);

    /**********************Get the CPU's Feature***********************/
    int fv = r.edx;
    TEST_FEATURE(fv, 1, "fpu")
    TEST_FEATURE(fv, 2, "vme")
    TEST_FEATURE(fv, 3, "de")
    TEST_FEATURE(fv, 4, "pse")
    TEST_FEATURE(fv, 5, "msr")
    TEST_FEATURE(fv, 6, "pae")
    TEST_FEATURE(fv, 7, "mce")
    TEST_FEATURE(fv, 8, "cxs")
    TEST_FEATURE(fv, 9, "apic")
    TEST_FEATURE(fv, 10, "Reserved")
    TEST_FEATURE(fv, 11, "SYSENTER/SYSEXIT")
    TEST_FEATURE(fv, 12, "mttr")
    TEST_FEATURE(fv, 13, "pge")
    TEST_FEATURE(fv, 14, "mca")
    TEST_FEATURE(fv, 15, "cmov")
    TEST_FEATURE(fv, 16, "pat")
    TEST_FEATURE(fv, 17, "pse-36")
    TEST_FEATURE(fv, 18, "psn")
    TEST_FEATURE(fv, 19, "clflush")
    // TEST_FEATURE(fv, 20, "Reserved")
    TEST_FEATURE(fv, 21, "dts")
    TEST_FEATURE(fv, 22, "acpi")
    TEST_FEATURE(fv, 23, "mmx")
    TEST_FEATURE(fv, 24, "fxsr")
    TEST_FEATURE(fv, 25, "sse")
    TEST_FEATURE(fv, 26, "sse2")
    // TEST_FEATURE(fv, 27, "Reserved")
    TEST_FEATURE(fv, 28, "ht")
    TEST_FEATURE(fv, 29, "tm")
    // TEST_FEATURE(fv, 30, "Reserved")
    TEST_FEATURE(fv, 31, "pbe")

    printk("\n");

    if (!((1UL << 11) & fv)) {
        printk("Your CPU Do Not Support SYSENTER/SYSEXIT\n");
        while (1)
            ;
    }
}

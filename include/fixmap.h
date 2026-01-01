/*
 * ------------------------------------------------------------------------
 *   File Name: fixmap.h
 *      Author: Zhao Yanbai
 *              2025-12-31 23:10:32 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <kdef.h>
#include <types.h>
#include <page.h>

#define MAX_IO_APIC_CNT (16)

enum {
    FIX_BGN_PAD,
    FIX_BGN_PAD_0 = FIX_BGN_PAD,
    FIX_BGN_PAD_1,

    // 在SMP系统中，即便有多个CPU，每个CPU都有一个LAPIC，但他们都共享同一个LAPIC物理地址
    // 虽然每个CPU都访问同样的物理地址，但实际上硬件会根据发起请求的CPU核心，自动路由到对应的本地APIC。
    FIX_LAPIC_BASE,

    FIX_IO_APIC_BASE,
    FIX_IO_APIC_END = FIX_IO_APIC_BASE + MAX_IO_APIC_CNT - 1,

    FIX_END_PAD,
};

void set_fixmap(uint32_t fixid, paddr_t paddr);

vaddr_t fixid_to_vaddr(uint32_t fixid);
uint32_t vaddr_to_fixid(vaddr_t vaddr);

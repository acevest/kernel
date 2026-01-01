/*
 * ------------------------------------------------------------------------
 *   File Name: fixmap.c
 *      Author: Zhao Yanbai
 *              2025-12-31 23:10:37 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <fixmap.h>
#include <assert.h>
#include <printk.h>

#define FIX_MAP_VADDR_REAL_END (FIX_MAP_VADDR_BASE + (FIX_END_PAD + 1) * PAGE_SIZE)

void fixmap_init() {
    // TODO
}

typedef struct _fixmap {
    paddr_t paddr;
    vaddr_t vaddr;
} fixmap_t;

fixmap_t fixmap[FIX_END_PAD + 1] = {0};

void set_fixmap(uint32_t fixid, paddr_t paddr) {
    assert(fixid >= FIX_BGN_PAD);
    assert(fixid <= FIX_END_PAD);

    vaddr_t vaddr = fixid_to_vaddr(fixid);

    set_pte_paddr(vaddr, paddr, PAGE_P | PAGE_WR);

#if 1
    fixmap[fixid].paddr = paddr;
    fixmap[fixid].vaddr = vaddr;
#endif
}

vaddr_t fixid_to_vaddr(uint32_t fixid) {
    assert(fixid >= FIX_BGN_PAD);
    assert(fixid <= FIX_END_PAD);

    return FIX_MAP_VADDR_BASE + fixid * PAGE_SIZE;
}

uint32_t vaddr_to_fixid(vaddr_t vaddr) {
    assert(vaddr >= FIX_MAP_VADDR_BASE);
    assert(vaddr < FIX_MAP_VADDR_END);
    assert(vaddr < FIX_MAP_VADDR_REAL_END);

    uint32_t fixid = ((vaddr & PAGE_MASK) - FIX_MAP_VADDR_BASE) / PAGE_SIZE;

    return fixid;
}

void dump_fixmap() {
    for (int i = 0; i <= FIX_END_PAD; i++) {
        fixmap_t* fmi = &fixmap[i];
        if (fmi->paddr == 0 || fmi->vaddr == 0) {
            continue;
        }
        printk("fixmap[%d]: map 0x%08x to kernel vaddr 0x%08x\n", i, fmi->paddr, fmi->vaddr);
    }
}

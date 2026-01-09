/*
 * ------------------------------------------------------------------------
 *   File Name: hpet.c
 *      Author: Zhao Yanbai
 *              2026-01-08 15:59:34 Thursday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <system.h>
#include <ioremap.h>
#include <fixmap.h>
#include <apic.h>
#include <assert.h>
#include <irq.h>

const static paddr_t hpet_phys_addrs[] = {
    0xFED00000,
    0xFED01000,
    0xFED02000,
    0xFED03000,
};
const static int hpet_use_phys_addr_index = 0;

const static int hpet_trigger_mode = IOAPIC_TRIGGER_MODE_EDGE;

static paddr_t hpet_phys_addr = 0;

static uint32_t hpet_timer_count = 0;

void hpet_hw_enable() {
    uint32_t map_offset = 3 * PAGE_SIZE;
    vaddr_t rcba_phys_base = (vaddr_t)get_rcba_paddr();

    vaddr_t rcba_virt_base = (vaddr_t)ioremap(rcba_phys_base + map_offset, 4 * PAGE_SIZE - map_offset);

    printk("RCBA %08x %08x mapped to %08x\n", rcba_phys_base, rcba_phys_base + map_offset, rcba_virt_base);

    // HPTC: High Precision Timer Control Register
    // bit[1:0] 地址映射范围选择域
    //       取值      地址映射范围
    //        00: 0xFED00000 - 0xFED003FF
    //        01: 0xFED01000 - 0xFED013FF
    //        10: 0xFED02000 - 0xFED023FF
    //        11: 0xFED03000 - 0xFED033FF
    // bit[7] 地址映射使能标志位，用于控制HPET设备访问地址的开启与否
    //        只有它置位时芯片组才会将HPET配置寄存器映射到内存空间
    uint32_t* pHPTC = (uint32_t*)((uint8_t*)rcba_virt_base + 0x3404 - map_offset);
    printk("HPTC: %08x %08x\n", *pHPTC, pHPTC);
    *pHPTC = *pHPTC | (1 << 7) | (hpet_use_phys_addr_index << 0);
    io_mfence();
    printk("HPTC: %08x\n", *pHPTC);
    iounmap(rcba_virt_base);
}

vaddr_t hpet_base() {
    return fixid_to_vaddr(FIX_HPET_BASE);
}

uint64_t hpet_read(uint32_t regoffset) {
    uint64_t value = *(uint64_t*)(hpet_base() + regoffset);
    io_mfence();
    return value;
}

uint64_t hpet_write(uint32_t regoffset, uint64_t value) {
    *(volatile uint64_t*)(hpet_base() + regoffset) = value;
    io_mfence();
    return value;
}

#define HPET_REG_CAPABILITY_ID 0x0
#define HPET_REG_CONFIG 0x10
#define HPET_REG_INTERRUPT_STATUS 0x20
#define HPET_REG_MAIN_COUNTER_VALUE 0xF0
#define HPET_REG_TIMn_CONFIG_CAPABILITY(n) (0x100 + (n) * 0x20)
#define HPET_REG_TIMn_COMPARATOR(n) (0x108 + (n) * 0x20)
#define HPET_REG_TIMn_FSB_INTERRUPT_ROUTE(n) (0x110 + (n) * 0x20)

void hpet_enable() {
    hpet_write(HPET_REG_CONFIG, hpet_read(HPET_REG_CONFIG) | (1ULL << 0));
}

void hpet_disable() {
    hpet_write(HPET_REG_CONFIG, hpet_read(HPET_REG_CONFIG) & ~(1ULL << 0));
}

uint32_t hpet_clock_period = 0;
uint64_t hpet_clock_mhz_freq = 0;  // 32位系统不支持64位除法，所以用MHz为单位

static uint64_t hpet_ticks = 0;
void hpet0_bh_handler() {
    // hpet_ticks++;
    printlxy(MPL_IRQ, MPO_HPET, "HPET: %lu", hpet_ticks);
}
void hpet0_irq_handler(unsigned int irq, pt_regs_t* regs, void* dev_id) {
    hpet_ticks++;

    uint8_t* p = (uint8_t*)0xC00B8000;
    *p = *p == ' ' ? 'K' : ' ';

    add_irq_bh_handler(hpet0_bh_handler, NULL);

    system.lapic->write(LAPIC_EOI, 0);
}

uint64_t hpet_get_tick_counter(uint32_t hz) {
    assert(hz > 0);
    assert(hpet_clock_mhz_freq > 0);

    uint32_t r = 1000000 / hz;

    return r * hpet_clock_mhz_freq;
}

extern irq_chip_t ioapic_chip;

void hpet_init_timer0(uint32_t hz) {
    //
    hpet_disable();

    //
    uint32_t irq = 0;
    uint32_t ioapic_irq = 23;

    //
    uint32_t cpu_irq_vec = 0;
    request_irq(cpu_irq_vec, hpet0_irq_handler, "HPET#0", "HPET#0");

    uint64_t dst_cpuid = 0;
    ioapic_rte_t rte;
    printk("sizeof(ioapic_rte_t): %d\n", sizeof(ioapic_rte_t));
    assert(sizeof(ioapic_rte_t) == 8);
    rte.value = 0;
    rte.vector = 0x20 + irq;
    rte.delivery_mode = IOAPIC_DELIVERY_MODE_FIXED;
    rte.destination_mode = IOAPIC_PHYSICAL_DESTINATION;
    rte.trigger_mode = hpet_trigger_mode;
    rte.mask = IOAPIC_INT_UNMASKED;
    rte.destination = dst_cpuid;

    printk("HPET#0 IOAPIC RTE VALUE %08x\n", rte.value);

    ioapic_rte_write(IOAPIC_RTE(ioapic_irq), rte.value);
    irq_set_chip(irq, &ioapic_chip);

    uint64_t counter = hpet_get_tick_counter(hz);

    // 配置HPET#0
    uint64_t tim0_config = 0;
    tim0_config |= (hpet_trigger_mode << 1);
    tim0_config |= (1 << 2);  // enable interrupt
    tim0_config |= (1 << 3);  // periodic
    tim0_config |= (1 << 5);  // 64bit
    // tim0_config |= (1 << 6);           // ....
    tim0_config |= (ioapic_irq << 9);  // ioapic irq

    printk("TIM0_CONF: 0x%08x%08x\n", (uint32_t)(tim0_config >> 32), (uint32_t)tim0_config);

    hpet_write(HPET_REG_TIMn_CONFIG_CAPABILITY(0), tim0_config);

    hpet_write(HPET_REG_TIMn_COMPARATOR(0), counter);

    hpet_write(HPET_REG_MAIN_COUNTER_VALUE, 0);

    hpet_enable();
}

void hpet_prepare_calibration(uint32_t timn, uint32_t hz) {
    assert(timn < hpet_timer_count);
    //
    // hpet_disable();

    uint64_t counter = hpet_get_tick_counter(hz);

    // 配置HPET#0
    uint64_t timn_config = 0;
    timn_config |= (hpet_trigger_mode << 1);
    timn_config |= (0 << 2);  // enable interrupt
    timn_config |= (0 << 3);  // periodic
    timn_config |= (1 << 5);  // 64bit
    // timn_config |= (1 << 6);           // ....
    // timn_config |= (ioapic_irq << 9);  // ioapic irq

    printk("TIM0_CONF: 0x%08x%08x\n", (uint32_t)(timn_config >> 32), (uint32_t)timn_config);

    hpet_write(HPET_REG_TIMn_CONFIG_CAPABILITY(timn), timn_config);

    hpet_write(HPET_REG_TIMn_COMPARATOR(timn), counter);

    hpet_write(HPET_REG_MAIN_COUNTER_VALUE, 0);

    // hpet_enable();
}

bool hpet_calibration_end(uint32_t timn) {
    if (hpet_read(HPET_REG_MAIN_COUNTER_VALUE) >= hpet_read(HPET_REG_TIMn_COMPARATOR(timn))) {
        return true;
    }
    return false;
}

void hpet_init() {
    assert(hpet_use_phys_addr_index < sizeof(hpet_phys_addrs) / sizeof(hpet_phys_addrs[0]));
    hpet_phys_addr = hpet_phys_addrs[hpet_use_phys_addr_index];

    set_fixmap(FIX_HPET_BASE, hpet_phys_addr);
    printk("HPET base %08x mapped to %08x\n", hpet_phys_addr, hpet_base());

    //
    hpet_hw_enable();

    uint64_t capid = hpet_read(HPET_REG_CAPABILITY_ID);
    hpet_clock_period = capid >> 32;
    hpet_clock_mhz_freq = 1000000000U / hpet_clock_period;  // 32位除法
    hpet_timer_count = ((capid >> 8) & 0x1F) + 1;
    printk("HPET Capability and ID: 0x%08x%08x\n", (uint32_t)(capid >> 32), (uint32_t)capid);
    printk("HPET legacy replacement route capable: %s\n", (capid & (1ULL << 15)) ? "Y" : "N");
    printk("HPET 64bit capable: %s\n", (capid & (1ULL << 13)) ? "Y" : "N");
    printk("HPET timer count: %d\n", hpet_timer_count);
    printk("HPET clock period: %u ns\n", hpet_clock_period);
    printk("HPET clock frequency: %u MHz\n", hpet_clock_mhz_freq);

    uint64_t config = hpet_read(HPET_REG_CONFIG);
    printk("HPET Configuration: 0x%08x%08x\n", (uint32_t)(config >> 32), (uint32_t)config);
    printk("HPET enabled: %s\n", (config & (1ULL << 0)) ? "Y" : "N");
    printk("HPET legacy replacement: %s\n", (config & (1ULL << 1)) ? "Y" : "N");
}

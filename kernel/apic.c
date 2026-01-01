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
#include <fixmap.h>
#include <apic.h>

static inline uint32_t apic_offset_to_msr(uint32_t offset) {
    return 0x800 + (offset >> 4);
}

uint32_t apic_read_lapic(uint32_t offset) {
    assert(offset < PAGE_SIZE);
    uint8_t* base = (uint8_t*)fixid_to_vaddr(FIX_LAPIC_BASE);
    return *(uint32_t*)(base + offset);
}

void apic_write_lapic(uint32_t offset, uint32_t value) {
    assert(offset < PAGE_SIZE);
    uint8_t* base = (uint8_t*)fixid_to_vaddr(FIX_LAPIC_BASE);
    *(uint32_t*)(base + offset) = value;
}

uint32_t apic_get_lapic_id() {
    // return lapic_read(LAPIC_ID) >> 24;
    return apic_read_lapic(LAPIC_ID) >> 24;
}

static lapic_t apic_lapic = {
    .name = "apic - lapic",
    .read = apic_read_lapic,
    .write = apic_write_lapic,
    .get_lapic_id = apic_get_lapic_id,
};

uint32_t x2apic_read_lapic(uint32_t offset) {
    assert(offset < PAGE_SIZE);
    uint32_t msr = apic_offset_to_msr(offset);
    return read_msr32(msr);
}

void x2apic_write_lapic(uint32_t offset, uint32_t value) {
    assert(offset < PAGE_SIZE);
    uint32_t msr = apic_offset_to_msr(offset);
    write_msr32(msr, value);
}

uint32_t x2apic_get_lapic_id() {
    uint32_t msr = apic_offset_to_msr(LAPIC_ID);
    return read_msr32(msr);
}

static lapic_t x2apic_lapic = {
    .name = "x2apic - lapic",
    .read = x2apic_read_lapic,
    .write = x2apic_write_lapic,
    .get_lapic_id = x2apic_get_lapic_id,
};

void lapic_lvt_detect() {
    lapic_t* lapic = system.lapic;
    struct {
        uint32_t offset;
        char* name;
    } lvt[] = {
        {LAPIC_LVT_CMCI, "CMCI"},        //
        {LAPIC_LVT_TIMER, "TIMER"},      //
        {LAPIC_LVT_THERMAL, "THERMAL"},  //
        {LAPIC_LVT_PERF, "PERF"},        //
        {LAPIC_LVT_LINT0, "LINT0"},      //
        {LAPIC_LVT_LINT1, "LINT1"},      //
        {LAPIC_LVT_ERROR, "ERROR"},      //
    };

    for (int i = 0; i < sizeof(lvt) / sizeof(lvt[0]); i++) {
        uint32_t lvt_value = lapic->read(lvt[i].offset);
        printk("LVT[%d] %s: %08x\n", i, lvt[i].name, lvt_value);
    }
}

void lapic_init() {
    bool x2apic = false;
    cpuid_regs_t r;
    r = cpuid(1);
    if (r.edx & (1 << 9)) {
        printk("local apic supported\n");
        if (r.ecx & (1 << 21)) {
            printk("x2apic supported\n");
        } else {
            panic("x2apic not supported\n");
        }
    } else {
        panic("local apic not supported\n");
    }

    uint32_t apic_base = read_msr32(MSR_IA32_APIC_BASE);
    printk("LAPIC BASE: %08x\n", apic_base);

    // apic 必然已经开启
    assert((apic_base & (1 << 11)) != 0);

    // 映射LAPIC到内核空间
    uint32_t apic_phys_base_addr = (uint32_t)apic_base & PAGE_MASK;
    set_fixmap(FIX_LAPIC_BASE, apic_phys_base_addr);
    //
    vaddr_t apic_virt_base_addr = fixid_to_vaddr(FIX_LAPIC_BASE);
    printk("LAPIC base %08x mapped to %08x\n", apic_phys_base_addr, apic_virt_base_addr);

    if (x2apic) {
        // 开启2xapic
        apic_base |= (1 << 10);
        write_msr(MSR_IA32_APIC_BASE, apic_base);

        apic_base = read_msr32(MSR_IA32_APIC_BASE);
        assert((apic_base & (1 << 10)) != 0);
        printk("after 2xapic enable apic base: %016lx\n", apic_base);

        system.lapic = &x2apic_lapic;
    } else {
        system.lapic = &apic_lapic;
    }

    lapic_t* lapic = system.lapic;

    uint32_t version = lapic->read(LAPIC_VERSION);
    uint32_t id = lapic->get_lapic_id();
    uint32_t lvt_cnt = ((version >> 16) & 0xff) + 1;

    printk("LAPIC id %08x version %08x lvt_cnt %d\n", id, version, lvt_cnt);
    if ((version & 0xFF) < 0x10) {
        printk(" Intel 82489DX APIC\n");
    } else {
        printk(" Integrated APIC\n");
    }

    // 在LAPIC_SVR中开启LAPIC同时禁用EOI广播
    uint32_t lapic_svr = lapic->read(LAPIC_SVR);
    printk("LAPIC_SVR: %08x\n", lapic_svr);
    lapic_svr |= (1 << 8);    // 启用LAPIC
    lapic_svr &= ~(1 << 12);  // 禁用EOI广播

    lapic->write(LAPIC_SVR, lapic_svr);

    lapic_svr = lapic->read(LAPIC_SVR);
    printk("LAPIC_SVR: %08x\n", lapic_svr);
    assert((lapic_svr & (1 << 8)) != 0);
    assert((lapic_svr & (1 << 12)) == 0);
    printk("LAPIC enabled and EOI broadcast disabled\n");

    lapic_lvt_detect();

    // 屏蔽LVT所有中断功能
    uint32_t lvt_mask = 0x10000;
    // lapic->write(LAPIC_LVT_CMCI, lvt_mask);
    lapic->write(LAPIC_LVT_TIMER, lvt_mask);
    lapic->write(LAPIC_LVT_THERMAL, lvt_mask);
    lapic->write(LAPIC_LVT_PERF, lvt_mask);
    lapic->write(LAPIC_LVT_LINT0, lvt_mask);
    lapic->write(LAPIC_LVT_LINT1, lvt_mask);
    lapic->write(LAPIC_LVT_ERROR, lvt_mask);

    // lapic_lvt_detect();

    // TPR: Task Priority Register
    // bit[7:4]: 任务优先级
    // bit[3:0]: 任务子优先级，新版本处理要求为0
    lapic->write(LAPIC_TPR, 0);
    uint32_t TPR = lapic->read(LAPIC_TPR);

    // PPR: Processor Priority Register
    // bit[7:4]: 处理器优先级
    // bit[3:0]: 处理器子优先级
    // 只读寄存器
    // PPR是TPR和ISRV相比较得到的。ISRV代表ISR(In-Service Register共256bit)寄存器当前服务的最高优先级中断的向量号
    // PPR = max(TPR, (ISRV != 0) ? (ISRV & 0xF0) : 0)
    // 这里的含义是: 处理器当前的优先级水平（PPR）取软件设置的任务优先级（TPR）​
    // 和当前正在服务的中断的优先级（ISRV的高4位）​ 中的较大者
    // 当一个高优先级中断正在被服务时（ISRV值大），即使TPR设置得很低，PPR也会保持高位，从而阻止同级或更低级的中断嵌套。
    // TPR是软件的“计划”，ISR是硬件的“现状”，而PPR是综合二者得出的“当前执行标准”。
    // 内核只能通过设置TPR来影响PPR
    uint32_t PPR = lapic->read(LAPIC_PPR);
}

void init_apic() {
    ioapic_init();
}

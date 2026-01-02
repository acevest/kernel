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
#include <pci.h>
#include <ioremap.h>
#include <io.h>
#include <irq.h>

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
        // {LAPIC_LVT_CMCI, "CMCI"},        //
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
            x2apic = true;
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
        printk("after 2xapic enable apic base: %08x\n", apic_base);
        write_msr(MSR_IA32_APIC_BASE, apic_base);
        printk("after 2xapic enable apic base: %08x\n", apic_base);

        apic_base = read_msr32(MSR_IA32_APIC_BASE);
        assert((apic_base & (1 << 10)) != 0);
        printk("after 2xapic enable apic base: %08x\n", apic_base);

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

static ioapic_map_t ioapic_map;

uint64_t ioapic_rte_read(uint32_t index) {
    uint64_t v = 0;
    *(volatile uint32_t*)ioapic_map.io_reg_sel = index + 1;
    io_mfence();
    v = *(volatile uint32_t*)ioapic_map.io_win;
    io_mfence();

    v <<= 32;

    *(volatile uint32_t*)ioapic_map.io_reg_sel = index + 0;
    io_mfence();
    v |= *(volatile uint32_t*)ioapic_map.io_win;
    io_mfence();

    return v;
}

void ioapic_rte_write(uint32_t index, uint64_t v) {
    *(volatile uint32_t*)ioapic_map.io_reg_sel = index + 0;
    io_mfence();
    *(volatile uint32_t*)ioapic_map.io_win = v & 0xFFFFFFFF;
    io_mfence();

    v >>= 32;

    *(volatile uint32_t*)ioapic_map.io_reg_sel = index + 1;
    io_mfence();
    *(volatile uint32_t*)ioapic_map.io_win = v & 0xFFFFFFFF;
    io_mfence();
}

void ioapic_init() {
    // 先找到RCBA: Root Complex Base Address寄存器
    uint32_t cmd = PCI_CMD(0, 31, 0, 0xF0);
    printk("CMD: %08x\n", cmd);
    uint32_t RCBA = pci_read_config_long(cmd);

    if ((RCBA & 1) == 0) {
        panic("RCBA not enabled\n");
    }

    // 把IO APIC映射进地址空间
    ioapic_map.phys_base = system.ioapic_addr;
    ioapic_map.io_reg_sel = (vaddr_t)ioremap(ioapic_map.phys_base, PAGE_SIZE);
    ioapic_map.io_win = ioapic_map.io_reg_sel + 0x10;
    ioapic_map.eoi = ioapic_map.io_reg_sel + 0x40;
    assert(ioapic_map.phys_base != 0);
    assert(ioapic_map.io_reg_sel != 0);
    printk("IO-APIC mapped %08x to %08x\n", ioapic_map.phys_base, ioapic_map.io_reg_sel);
    printk("IO-APIC io_win %08x eoi %08x\n", ioapic_map.io_win, ioapic_map.eoi);

    system.ioapic_map = &ioapic_map;

    // IO APIC ID
    *(volatile uint32_t*)ioapic_map.io_reg_sel = IOAPIC_ID;
    io_mfence();
    uint32_t ioapic_id = *(volatile uint32_t*)ioapic_map.io_win;
    io_mfence();

    // IO APIC VERSION
    *(volatile uint32_t*)ioapic_map.io_reg_sel = IOAPIC_VERSION;
    io_mfence();
    uint32_t ioapic_version = *(volatile uint32_t*)ioapic_map.io_win;
    io_mfence();

    int rte_cnt = ((ioapic_version >> 16) & 0xFF) + 1;
    printk("IO-APIC id %08x version %08x RTE cnt %d\n", ioapic_id, ioapic_version, rte_cnt);

    // 屏蔽所有中断
    for (int i = 0; i < rte_cnt; i++) {
        uint32_t irq = 0x20 + i;
        ioapic_rte_write(IOAPIC_RTE(i), IOAPIC_RTE_MASK | irq);
    }

    // RCBA
    // bit[0]: 使能位
    // bit[13:1]: 保留
    // bit[31:14]: RCBA物理基地址
    // 0x3FFF == (1 << 14) - 1
    uint32_t rcba_phys_base = RCBA & (~0x3FFF);

    printk("RCBA: %08x %08x\n", RCBA, rcba_phys_base);

    // 把RCBA物理基地址映射到内核空间
    vaddr_t rcba_virt_base = (vaddr_t)ioremap(rcba_phys_base, 4 * PAGE_SIZE);
    // set_fixmap(FIX_RCBA_BASE, rcba_phys_base);
    // uint32_t rcba_virt_base = fixid_to_vaddr(FIX_RCBA_BASE);
    printk("RCBA base %08x mapped to %08x\n", rcba_phys_base, rcba_virt_base);

    // OIC
    // 位于RCBA 的 0x31FE 偏移处，是一个16位寄存器
    // bit[7:0]: APIC映射区。决定了IO APIC的间接访问寄存器的地址区间。只有在禁用IO APIC的情况下才能修。
    // bit[8]: IO APIC使能标志位。 置位使能 复位禁用
    // bit[9] 协处理器错误使能标志位
    // bit[15:10]: 保留
    //
    // 上面得搞成ioremap映射, 因为0x31FE这个超过一页也就是4K了。
    uint16_t* pOIC = (uint16_t*)((uint8_t*)rcba_virt_base + 0x31FE);
    printk("OIC: %04x\n", *pOIC);
    *pOIC = *pOIC | (1 << 8);
    printk("OIC: %04x\n", *pOIC);
    // TODO
    // iounmap(rcba_virt_base);

    // 打开键盘中断
    ioapic_rte_write(IOAPIC_RTE(1), 0x21);
    extern irq_chip_t ioapic_chip;
    irq_set_chip(0x01, &ioapic_chip);
}

void disable_i8259();
void init_apic() {
    // mask_i8259();
    disable_i8259();
    // imcr_init();
    lapic_init();
    ioapic_init();
}

// ## 中断路由路径配置矩阵

// | 路径 | IMCR bit0 | 8259A | LAPIC | IOAPIC | 中断引脚连接 |
// |------|-----------|-------|-------|--------|------------|
// | **路径A** | 0 (PIC) | 启用 | 禁用 | 禁用/忽略 | INTR直接到CPU |
// | **路径B** | 1 (APIC) | 启用 | 启用 | 禁用 | INTR到LINT0 |
// | **路径C** | 1 (APIC) | 启用 | 启用 | 启用 | 8259A到IOAPIC |
// | **路径D** | 1 (APIC) | 禁用/掩码 | 启用 | 启用 | 直连IOAPIC |

// ### 说明：
// - **IMCR bit0**: 0 = PIC模式, 1 = APIC模式
// - **路径A**: 传统PIC模式，用于实模式/早期保护模式
// - **路径B**: 过渡模式，8259A中断通过LAPIC的LINT0引脚
// - **路径C**: 兼容模式，现代系统启动时常用
// - **路径D**: 现代标准模式，性能最优

// A. 8259直接到CPU
//  实模式/早期保护模式
//  中断向量表直接指向ISR
//  8259A通过INTR引脚直接触发CPU中断

// B. 8259 → LAPIC → CPU
//  本地APIC启用，但IOAPIC未启用
//  8259A中断通过LAPIC的LINTO引脚
//  需要配置LAPIC的LVT LINT0/1寄存器

// C. 8259 → IOAPIC → LAPIC → CPU
//  现代系统兼容模式
//  8259A中断连接到IOAPIC的IRQ0-15
//  IOAPIC重定向到LAPIC
//  需要IMCR寄存器配置

// D. IOAPIC → LAPIC → CPU
//  纯APIC模式
//  设备中断直接连接到IOAPIC
//  IOAPIC重定向到LAPIC
//  8259A被禁用或掩码

int enable_ioapic_irq(unsigned int irq) {
    uint64_t rte = ioapic_rte_read(irq);
    rte &= ~IOAPIC_RTE_MASK;
    ioapic_rte_write(irq, rte);
    return 0;
}

int disable_ioapic_irq(unsigned int irq) {
    uint64_t rte = ioapic_rte_read(irq);
    rte |= IOAPIC_RTE_MASK;
    ioapic_rte_write(irq, rte);
    return 0;
}

void ack_ioapic_irq(unsigned int irq) {
    system.lapic->write(LAPIC_EOI, 0);
}

irq_chip_t ioapic_chip = {
    .name = "IO-APIC",
    .enable = enable_ioapic_irq,
    .disable = disable_ioapic_irq,
    .ack = ack_ioapic_irq,
};

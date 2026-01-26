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
    uint32_t n = LAPIC_MSR_BASE + (offset >> 4);
    return n;
}

uint32_t apic_read_lapic(uint32_t offset) {
    assert(offset < PAGE_SIZE);
    uint8_t* base = (uint8_t*)fixid_to_vaddr(FIX_LAPIC_BASE);
    return *(volatile uint32_t*)(base + offset);
}

void apic_write_lapic(uint32_t offset, uint32_t value) {
    assert(offset < PAGE_SIZE);
    uint8_t* base = (uint8_t*)fixid_to_vaddr(FIX_LAPIC_BASE);
    *(volatile uint32_t*)(base + offset) = value;
}

void apic_write64_lapic(uint32_t offset, uint64_t value) {
    assert(offset < PAGE_SIZE);
    uint8_t* base = (uint8_t*)fixid_to_vaddr(FIX_LAPIC_BASE);
    *(volatile uint64_t*)(base + offset) = value;
}

uint32_t apic_get_lapic_id() {
    // return lapic_read(LAPIC_ID) >> 24;
    return apic_read_lapic(LAPIC_ID) >> 24;
}

static lapic_t apic_lapic = {
    .name = "apic - lapic",
    .x2apic = false,
    .read = apic_read_lapic,
    .write = apic_write_lapic,
    .write64 = apic_write64_lapic,
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

void x2apic_write64_lapic(uint32_t offset, uint64_t value) {
    assert(offset < PAGE_SIZE);
    uint32_t msr = apic_offset_to_msr(offset);
    write_msr(msr, value);
}

uint32_t x2apic_get_lapic_id() {
    uint32_t msr = apic_offset_to_msr(LAPIC_ID);
    return read_msr32(msr);
}

static lapic_t x2apic_lapic = {
    .name = "x2apic - lapic",
    .x2apic = true,
    .read = x2apic_read_lapic,
    .write = x2apic_write_lapic,
    .write64 = x2apic_write64_lapic,
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
            // panic("x2apic not supported\n");
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
        // system.lapic = &apic_lapic;
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
    printk("TPR: %08x PPR: %08x\n", TPR, PPR);
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

void ioapic_enable() {
    uint32_t rcba_phys_base = (uint32_t)get_rcba_paddr();

    uint32_t map_offset = 3 * PAGE_SIZE;
    vaddr_t rcba_virt_base = (vaddr_t)ioremap(rcba_phys_base + map_offset, 4 * PAGE_SIZE - map_offset);

    printk("RCBA %08x %08x mapped to %08x\n", rcba_phys_base, rcba_phys_base + map_offset, rcba_virt_base);

    // OIC
    // 位于RCBA 的 0x31FE 偏移处，是一个16位寄存器
    // bit[7:0]: APIC映射区。决定了IO APIC的间接访问寄存器的地址区间。只有在禁用IO APIC的情况下才能修改。
    // bit[8]: IO APIC使能标志位。 置位使能 复位禁用
    // bit[9] 协处理器错误使能标志位
    // bit[15:10]: 保留
    //
    uint16_t* pOIC = (uint16_t*)((uint8_t*)rcba_virt_base + 0x31FE - map_offset);
    printk("OIC: %04x\n", *pOIC);
    *pOIC = *pOIC | (1 << 8);
    printk("OIC: %04x\n", *pOIC);

    iounmap(rcba_virt_base);
}

void ioapic_eoi() {
    system.lapic->write(LAPIC_EOI, 0);
}

void ioapic_init() {
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

    // uint32_t rcba_phys_base = (uint32_t)get_rcba_paddr();

    ioapic_enable();

    // uint64_t dst_cpuid = 0;

    extern irq_chip_t ioapic_chip;
#if 1
    ioapic_rte_t rte;

    // 8253/8254连在8259的0号引脚，但8259连在IO APIC的2号引脚上
    // 把8253/8254的中断通过IOAPIC转发到CPU1的0号中断
    rte.value = 0;
    rte.vector = 0x20 + 0;
    rte.delivery_mode = IOAPIC_DELIVERY_MODE_FIXED;
    rte.destination_mode = IOAPIC_PHYSICAL_DESTINATION;
    rte.trigger_mode = IOAPIC_TRIGGER_MODE_EDGE;
    rte.mask = IOAPIC_INT_MASKED;  // 暂时先屏蔽
    rte.destination = 1;
    ioapic_rte_write(IOAPIC_RTE(2), rte.value);

    // 把键盘中断通过IOAPIC转发到CPU0的1号中断
    rte.value = 0;
    rte.vector = 0x20 + 1;
    rte.delivery_mode = IOAPIC_DELIVERY_MODE_FIXED;
    rte.destination_mode = IOAPIC_PHYSICAL_DESTINATION;
    rte.trigger_mode = IOAPIC_TRIGGER_MODE_EDGE;
    rte.mask = IOAPIC_INT_UNMASKED;
    rte.destination = 0;
    ioapic_rte_write(IOAPIC_RTE(1), rte.value);
    irq_set_chip(0x01, &ioapic_chip);

    //
    int irq = 0x0B;  // 11
    rte.value = 0;
    rte.vector = 0x20 + irq;
    rte.delivery_mode = IOAPIC_DELIVERY_MODE_FIXED;
    rte.destination_mode = IOAPIC_PHYSICAL_DESTINATION;
    rte.trigger_mode = IOAPIC_TRIGGER_MODE_EDGE;
    rte.mask = IOAPIC_INT_UNMASKED;
    rte.destination = 0;
    ioapic_rte_write(IOAPIC_RTE(irq), rte.value);
    irq_set_chip(irq, &ioapic_chip);

    extern void sata_irq_handler(unsigned int irq, pt_regs_t* regs, void* dev_id);
    request_irq(irq, sata_irq_handler, "SATA", "SATA");
#endif
}

void wakeup_ap(paddr_t paddr) {
    assert(PAGE_DOWN(paddr) == paddr);

    uint32_t pfn = PFN_DW(paddr);
    assert(pfn <= 0xFF);

    // 这里使用apic_lapic是因为ICR是一个64位寄存器
    // 如果用MSR读写的话，只能用0x830这个地址，是没有也不能用LAPIC_ICR_HIGH转换成的0x831的地址的
    // 而x2apic_lapic的write操作里的自动将偏移转换成MSR的逻辑目前是不支持屏蔽0x831的
    // lapic_t* lapic = &apic_lapic;
    lapic_t* lapic = system.lapic;

    uint64_t id = 0;
    id <<= 32;
    id <<= lapic->x2apic ? 0 : 24;  // 如果只是apic的话id需要左移56位

    // 在32位系统下ICR寄存器要先写高32位，再写低32位
    // 如果低32位数据写入那么处理器会立即发送IPI消息
    //
    // 这里为什么要单独写个write64?
    // 因为ICR是一个64位寄存器，对于apic的内存映射的方式没有问题，只要保证先写高32位再写低32位就可以了
    // 但是对于x2apic的MSR的情况就不一样了，因为ICR的MSR地址只有0x830，是没有也不能访问0x831的
    // 在lapic->write函数里只实现了按32位写数据
    // 所以如果尝试写LAPIC_ICR_HIGH的话，就会在x2apic_write_lapic里通过apic_offset_to_msr得到0x831的MSR地址
    // 所以就干脆写个写64位数据的函数

    // INIT IPI
    uint64_t init_ipi = id;
    init_ipi |= ((0b11) << 18);  // 向所有处理器发送消息(不包括自身)
    init_ipi |= ((0b0) << 15);   // 0=边沿触发  1=电平触发
    init_ipi |= ((0b1) << 14);   // 信号驱动电平: 0=无效 1=有效
    init_ipi |= ((0b0) << 12);   // 投递状态: 0=空闲 1=发送挂起
    init_ipi |= ((0b0) << 11);   // 目标模式: 0=物理模式 1=逻辑模式
    init_ipi |= ((0b101) << 8);  // 投递模式: 101=INIT, 110=START UP
    init_ipi |= ((0x00) << 0);   // 中断向量，在STARTUP IPI中代表地址0xMM00:0000 -> 0xMM00*16+0x0000=0xMM000
    lapic->write64(LAPIC_ICR, init_ipi);
    printk("INIT IPI %016x\n", init_ipi);

    for (int i = 0; i < 1000 * 10000; i++) {
        asm("nop");
    }

    // STARTUP IPI
    uint64_t startup_ipi = id;
    startup_ipi |= ((0b11) << 18);  // 向所有处理器发送消息(不包括自身)
    startup_ipi |= ((0b0) << 15);   // 0=边沿触发  1=电平触发
    startup_ipi |= ((0b1) << 14);   // 信号驱动电平: 0=无效 1=有效
    startup_ipi |= ((0b0) << 12);   // 投递状态: 0=空闲 1=发送挂起
    startup_ipi |= ((0b0) << 11);   // 目标模式: 0=物理模式 1=逻辑模式
    startup_ipi |= ((0b110) << 8);  // 投递模式: 101=INIT, 110=START UP
    startup_ipi |= ((pfn) << 0);    // 中断向量，在STARTUP IPI中代表地址0xMM00:0000 -> 0xMM00*16+0x0000=0xMM000
    lapic->write64(LAPIC_ICR, startup_ipi);
    printk("STARTUP[0] IPI %016x\n", startup_ipi);

    for (int i = 0; i < 1000 * 10000; i++) {
        asm("nop");
    }

#if 0
    // 在调试的时候，如果在ap_boot.S的入口处写hlt，它会hlt的AP继续执行
    // intel 要求至少发两次
    lapic->write64(LAPIC_ICR, startup_ipi);
    printk("STARTUP[1] IPI %016x\n", startup_ipi);
#endif

    printk("wakeup ap\n");
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

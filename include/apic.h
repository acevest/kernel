/*
 * ------------------------------------------------------------------------
 *   File Name: apic.h
 *      Author: Zhao Yanbai
 *              2026-01-01 16:08:19 Thursday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

// APIC的ID寄存器
// 在x2apic上，LAPIC_ID是32位
// 在Pentium 4和Xeon上，LAPIC_ID是8位，位置在[31:24]
// 早期Pentium和P6家族上，LAPIC_ID是4位，位置在[27:24]
#define LAPIC_ID 0x20

// APIC的版本寄存器
// bit[7:0][VERSION] APIC版本
//   - 0x0x Intel 82489DX (外部APIC芯片)
//   - 0x1x Integrated APIC (内部APIC芯片)
// bit[23:16][MAXLVT] 最大本地向量表 <- 此值+1代表LVT表项数
// bit[24][EOI] 禁止广播EOI消息标志位
#define LAPIC_VERSION 0x30

#define LAPIC_TPR 0x80
#define LAPIC_APR 0x90
#define LAPIC_PPR 0xA0
#define LAPIC_EOI 0xB0
#define LAPIC_LDR 0xD0

// Destination Format Register
// x2APIC模式下，不再支持平坦模式，仅剩集群模式，以至于x2APIC模式废除了DFR寄存器
#define LAPIC_DFR 0xE0

// 伪中断向量寄存器
// Spurious Interrupt Vector Register
// bit[7:0][VECTOR] 伪中断向量
// bit[8][Enable] 1启用LAPIC; 0禁用LAPIC
// bit[11:9][Reserved]
// bit[12][Suppress EOI Broadcast] 1禁用EOI广播; 0启用EOI广播
// bit[15:13][Reserved]
// bit[31:16][Reserved]
#define LAPIC_SVR 0xF0

// 以下三个寄存器都是256位，按如下偏移访问
// 0x00 [031:000]
// 0x10 [063:032]
// 0x20 [095:064]
// 0x30 [127:096]
// 0x40 [159:128]
// 0x50 [191:160]
// 0x60 [223:192]
// 0x70 [255:224]
#define LAPIC_ISR 0x100
#define LAPIC_TMR 0x180
#define LAPIC_IRR 0x200

#define LAPIC_ESR 0x280
#define LAPIC_LVT_CMCI 0x2F0
#define LAPIC_ICR_LOW 0x300
#define LAPIC_ICR_HIGH 0x310
#define LAPIC_LVT_TIMER 0x320
#define LAPIC_LVT_THERMAL 0x330
#define LAPIC_LVT_PERF 0x340
#define LAPIC_LVT_LINT0 0x350
#define LAPIC_LVT_LINT1 0x360
#define LAPIC_LVT_ERROR 0x370
#define LAPIC_TIMER_INITIAL 0x380
#define LAPIC_TIMER_COUNTER 0x390
#define LAPIC_TIMER_DIVIDE 0x3E0

typedef struct lapic {
    const char* name;

    uint32_t (*read)(uint32_t reg_offset);
    void (*write)(uint32_t reg_offset, uint32_t value);

    uint32_t (*get_lapic_id)();
} lapic_t;

// IO APIC

// IO APIC IOREGSEL 的索引值
// bit[23:0] Reserved
// bit[27:24] IO APIC ID
// bit[31:28] Reserved
#define IOAPIC_ID 0x00

// IO APIC 版本寄存器
// bit[7:0] IO APIC版本
// bit[15:8] Reserved
// bit[23:16] 可用RTE数 <- 此值+1代表RTE数
// bit[31:24] Reserved
#define IOAPIC_VERSION 0x01
// 0x02-0x0F Reserved
#define IOAPIC_REDTBL_BASE 0x10
// #define IOAPIC_REDTBL00 (IOAPIC_REDTBL_BASE + 0x00 * 0x02)
// #define IOAPIC_REDTBL01 (IOAPIC_REDTBL_BASE + 0x01 * 0x02)
// ...
// #define IOAPIC_REDTBL23 (IOAPIC_REDTBL_BASE + 0x17 * 0x02)
// 0x40-0xFF Reserved

#define IOAPIC_RTE(n) (IOAPIC_REDTBL_BASE + (n) * 0x02)

// 总共24个Redirection Table
// 每个2个字节，低字节索引低32位，高字节索引高32位
// 也就是索引总位宽为64位

// Redirect Table Entry
// bit[7:0] 中断向量号
// bit[10:8] 中断投递模式
// bit[11] 目标模式
// bit[12] 投递状态
// bit[13] 电平触发极性
// bit[14] 远程IRR标志位
// bit[15] 触发模式
// bit[16] 屏蔽标志位
// bit[55:17] Reserved
// bit[63:56] 中断投递目标
#define IOAPIC_RTE_MASK 0x00010000

typedef struct ioapic_map {
    paddr_t phys_base;
    vaddr_t io_reg_sel;  // 也是 virt_base
    vaddr_t io_win;
    vaddr_t eoi;
} ioapic_map_t;

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

/*
 *--------------------------------------------------------------------------
 *   File Name: msr.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Fri Jan  2 19:54:18 2009
 * Last Update: Fri Jan  2 19:54:18 2009
 *
 *--------------------------------------------------------------------------
 */

#ifndef _MSR_H
#define _MSR_H

#include <types.h>

// APIC的基地址寄存器
// bit[8][BSP] 指示当前处理器是否为引导处理器
// bit[11][EN] 控制APIC和xAPIC模式的开启与关闭
// bit[12:MAXPHYADDR] 用于配置APIC寄存器组的物理基地址
#define MSR_IA32_APIC_BASE 0x1B

#define MSR_SYSENTER_CS 0x174
#define MSR_SYSENTER_ESP 0x175
#define MSR_SYSENTER_EIP 0x176

#define MSR_IA32_PERF_STATUS 0x198
#define MSR_IA32_PERF_CRTL 0x199

// APIC的ID寄存器
#define MSR_IA32_X2APIC_APICID 0x802
// APIC的版本寄存器
// bit[7:0][VERSION] APIC版本
//   - 0x0x Intel 82489DX (外部APIC芯片)
//   - 0x1x Integrated APIC (内部APIC芯片)
// bit[23:16][MAXLVT] 最大本地向量表
// bit[24][EOI] 禁止广播EOI消息标志位
#define MSR_IA32_X2APIC_VERSION 0x803

#define rdmsr(msr, lowval, highval)                             \
    do {                                                        \
        asm("rdmsr;" : "=a"(lowval), "=d"(highval) : "c"(msr)); \
    } while (0)

#define wrmsr(msr, lowval, highval)                          \
    do {                                                     \
        asm("wrmsr;" ::"c"(msr), "a"(lowval), "d"(highval)); \
    } while (0)

static inline uint64_t read_msr(uint32_t msr) {
    uint32_t lowval = 0;
    uint32_t highval = 0;

    rdmsr(msr, lowval, highval);

    return ((uint64_t)highval << 32) | lowval;
}

static inline void write_msr(uint32_t msr, uint64_t value) {
    uint32_t lowval = value & 0xFFFFFFFF;
    uint32_t highval = value >> 32;

    wrmsr(msr, lowval, highval);
}

#endif  //_MSR_H

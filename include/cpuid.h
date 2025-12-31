/*
 * ------------------------------------------------------------------------
 *   File Name: include/cpuid.h
 *      Author: Zhao Yanbai
 *              2025-12-27 16:13:55 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

typedef struct cpuid_regs {
    unsigned long eax, ebx, ecx, edx;
} cpuid_regs_t;

cpuid_regs_t cpuid(unsigned long op);

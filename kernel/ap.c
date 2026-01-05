/*
 * ------------------------------------------------------------------------
 *   File Name: ap.c
 *      Author: Zhao Yanbai
 *              2026-01-04 20:15:33 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <page.h>

extern pde_t* ap_pre_pgd;

void ap_kernel_entry() {
    // 虽然ap_pre_pgd只做了一下跳板页目录看起来很可惜
    // 不过可以把它拿来个AP的栈用
    asm("mov %0, %%esp" : : "r"(pa2va(&ap_pre_pgd) + PAGE_SIZE));
    while (1) {
        asm("nop;");
    }
}

/*
 *--------------------------------------------------------------------------
 *   File Name: system.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Sat Feb  7 18:57:58 2009
 * Last Update: Sat Feb  7 18:57:58 2009
 *
 *--------------------------------------------------------------------------
 */

#ifndef _SYSTEM_H
#define _SYSTEM_H

#include <assert.h>
#include <page.h>
#define KRNLADDR PAGE_OFFSET

#define PT_REGS_EBX 0
#define PT_REGS_ECX 4
#define PT_REGS_EDX 8
#define PT_REGS_ESI 12
#define PT_REGS_EDI 16
#define PT_REGS_EBP 20
#define PT_REGS_EAX 24
#define PT_REGS_DS 28
#define PT_REGS_ES 32
#define PT_REGS_FS 36
#define PT_REGS_GS 40
#define PT_REGS_IRQ 44
#define PT_REGS_EIP 48
#define PT_REGS_CS 52
#define PT_REGS_EFLAGS 56
#define PT_REGS_ESP 60
#define PT_REGS_SS 64

#ifndef ASM
#include "printk.h"
#include "types.h"

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define typecheck(type, x)             \
    ({                                 \
        type __dummy;                  \
        typeof(x) __dummy2;            \
        (void)(&__dummy == &__dummy2); \
        1;                             \
    })

void *kmalloc(size_t size, gfp_t gfpflags);
void kfree(void *addr);

#define panic(msg, ...)                                                                                         \
    do {                                                                                                        \
        asm("cli;");                                                                                            \
        printk("PANIC:" msg " file:%s function:%s line:%d\n", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__); \
        while (1)                                                                                               \
            ;                                                                                                   \
    } while (0);

extern char etext, edata, end;

extern char gdtr[6], idtr[6];
#define lgdt() __asm__ __volatile__("lgdt gdtr")
#define sgdt() __asm__ __volatile__("sgdt gdtr")
#define lidt() __asm__ __volatile__("lidt idtr")
#define sidt() __asm__ __volatile__("sidt idtr")

#define cli() __asm__ __volatile__("cli")
#define sti() __asm__ __volatile__("sti")
#define nop() asm volatile("nop")
#define disableIRQ() cli()
#define enableIRQ() sti()

#define ALIGN(x, a) (((x) + (a)-1) & ~((a)-1))

// 定义最大显存为 16MB
#define VRAM_VADDR_SIZE (16 << 20)

// 最大支持的线性地址空间为1G
#define MAX_SUPT_VADDR_SIZE (1UL << 30)

// 把内核线性地址的最高部分留给显存
// 余下的部分为支持映射其它物理内存的空间
#define MAX_SUPT_PHYMM_SIZE (MAX_SUPT_VADDR_SIZE - VRAM_VADDR_SIZE)

// 算出显存的线性地址
// 之后要将这个地址映射到显存的物理地址
#define VRAM_VADDR_BASE (PAGE_OFFSET + MAX_SUPT_PHYMM_SIZE)

#define INT_STACK_SIZE PAGE_SIZE

enum GDTSelectorIndex {
    INDEX_SPACE = 0,
    INDEX_KCODE,
    INDEX_KDATA,
    INDEX_UCODE,
    INDEX_UDATA,
    INDEX_EMP1,
    INDEX_EMP2,
    INDEX_EMP3,
    INDEX_EMP4,
    INDEX_EMP5,
    INDEX_EMP6,
    INDEX_EMP7,
    INDEX_EMP8,
    INDEX_TSS,
};

typedef struct pt_regs {
    u32 ebx;
    u32 ecx;
    u32 edx;
    u32 edi;
    u32 esi;
    u32 ebp;
    u32 eax;
    u16 ds, _ds;
    u16 es, _es;
    u16 fs, _fs;
    u16 gs, _gs;
    union {
        u32 irq;
        u32 errcode;
    };
    u32 eip;
    u16 cs, _cs;
    u32 eflags;
    u32 esp;
    u16 ss, _ss;
} __attribute__((packed)) pt_regs_t;

typedef unsigned long dev_t;

typedef struct system {
    u32 mmap_addr;
    u32 mmap_size;  // Byte

    u32 mm_lower;  // KB
    u32 mm_upper;  // KB
    u64 mm_size;   // Byte

    u32 page_count;

    u32 page_bitmap;

    u32 *page_dir;
    u32 *pte_start;
    u32 *pte_end;

    void *kernel_begin;
    void *kernel_end;
    void *bootmem_bitmap_begin;

    // +-------+-------+-------+-------+
    // | drive | part1 | part2 | part3 |
    // +-------+-------+-------+-------+
    // Partition numbers always start from zero.
    // Unused partition bytes must be set to 0xFF.
    // More Infomation see 'info multiboot'
    u32 boot_device;

    u32 vbe_phys_addr;
    u16 x_resolution;
    u16 y_resolution;

    dev_t root_dev;
#define CMD_LINE_SIZE 128
    const char *cmdline;

    u32 debug;

    u32 delay;
} System, *pSystem;

extern System system;

void system_delay();

#define pgmap system.page_map

#endif

#define SAVE_REGS \
    cld;          \
    pushl % gs;   \
    pushl % fs;   \
    pushl % es;   \
    pushl % ds;   \
    pushl % eax;  \
    pushl % ebp;  \
    pushl % esi;  \
    pushl % edi;  \
    pushl % edx;  \
    pushl % ecx;  \
    pushl % ebx;

#define RESTORE_REGS \
    popl % ebx;      \
    popl % ecx;      \
    popl % edx;      \
    popl % edi;      \
    popl % esi;      \
    popl % ebp;      \
    popl % eax;      \
    popl % ds;       \
    popl % es;       \
    popl % fs;       \
    popl % gs;

#define PRIVILEGE_KRNL 0x0
#define PRIVILEGE_USER 0x3

#define INDEX_UCODE 3
#define INDEX_UDATA 4
/* *8 == <<3 . but <<3 is not right for asm code. */
#define SELECTOR_KRNL_CS (INDEX_KCODE * 8)
#define SELECTOR_KRNL_DS (INDEX_KDATA * 8)
#define SELECTOR_KRNL_SS SELECTOR_KRNL_DS
#define SELECTOR_USER_CS ((INDEX_UCODE * 8) | PRIVILEGE_USER)
#define SELECTOR_USER_DS ((INDEX_UDATA * 8) | PRIVILEGE_USER)
#define SELECTOR_USER_SS SELECTOR_USER_DS

#if 0
#define INT_VECT_DIVIDE 0x0
#define INT_VECT_DEBUG 0x1
#define INT_VECT_NMI 0x2
#define INT_VECT_BREAKPOINT 0x3 /* Break Point */
#define INT_VECT_OVERFLOW 0x4
#define INT_VECT_BOUNDS 0x5
#define INT_VECT_INVALOP 0x6
#define INT_VECT_COPROCNOT 0x7
#define INT_VECT_DOUBLEFAULT 0x8 /* Double Fault */
#define INT_VECT_COPROCSEG 0x9
#define INT_VECT_INVALTSS 0xA
#define INT_VECT_SEGNOT 0xB
#define INT_VECT_STACKFAULT 0xC /* Stack Fault */
#define INT_VECT_PROTECTION 0xD
#define INT_VECT_PAGEFAULT 0xE
#define INT_VECT_COPROCERR 0x10
#endif

#define INT_VECT_IRQ0 0x20
#define INT_VECT_IRQ8 0x28

#define REBOOT_RESTART 0x00
#define REBOOT_POWEROFF 0x01

#define KRNL_INIT_STACK_SIZE 4096

#endif  //_SYSTEM_H

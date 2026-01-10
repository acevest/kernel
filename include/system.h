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
#include <kdef.h>

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
#include <apic.h>

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define typecheck(type, x)             \
    ({                                 \
        type __dummy;                  \
        typeof(x) __dummy2;            \
        (void)(&__dummy == &__dummy2); \
        1;                             \
    })

void* kmalloc(size_t size, gfp_t gfpflags);
void* kzalloc(size_t size, gfp_t gfpflags);
void kfree(void* addr);

extern char etext, edata, end;

#define cli() asm volatile("cli")
#define sti() asm volatile("sti")
#define nop() asm volatile("nop")
#define mb() asm volatile("" ::: "memory")
#define disableIRQ() cli()
#define enableIRQ() sti()

#define ALIGN(x, a) (((x) + (a) - 1) & ~((a) - 1))

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

typedef uint32_t dev_t;

typedef struct system {
    u32 mmap_addr;
    u32 mmap_size;  // Byte

    u32 mm_lower;  // KB
    u32 mm_upper;  // KB
    u64 mm_size;   // Byte

    u32 page_count;

    u32 page_bitmap;

    u32* page_dir;
    u32* pte_start;
    u32* pte_end;

    void* kernel_begin;
    void* kernel_end;
    void* bootmem_bitmap_begin;

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

    void* rsdt_addr;

    dev_t root_dev;

    //
    uint32_t ap_cpuid;

    // 按理这些信息应该按CPU存储，简化实现
    lapic_t* lapic;
    paddr_t lapic_addr;

    //
    paddr_t ioapic_addr;
    ioapic_map_t* ioapic_map;

#define CMD_LINE_SIZE 128
    const char* cmdline;

    u32 debug;

    u32 delay;
} system_t;

extern system_t system;

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

#ifndef ASM
// 内核进程
void root_task_entry();
void init_task_entry();
void disk_task_entry();
void user_task_entry();

extern volatile int reenter;

#define DEFAULT_BOOT_DELAY_TICKS 300
void boot_delay(int ticks);

void io_mfence();

paddr_t get_rcba_paddr();
#endif

#define DISABLE_IDE 1

#define panic(msg, ...)                                                                      \
    do {                                                                                     \
        asm("cli;");                                                                         \
        printk("PANIC:" msg " %s %s %d\n", ##__VA_ARGS__, __FILE__, __FUNCTION__, __LINE__); \
        asm("hlt");                                                                          \
    } while (0);

#endif  //_SYSTEM_H

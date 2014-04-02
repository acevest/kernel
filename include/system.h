/*
 *--------------------------------------------------------------------------
 *   File Name:    system.h
 * 
 * Description:    none
 * 
 * 
 *      Author:    Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Sat Feb  7 18:57:58 2009
 * Last Update: Sat Feb  7 18:57:58 2009
 * 
 *--------------------------------------------------------------------------
 */

#ifndef    _SYSTEM_H
#define    _SYSTEM_H

#include<page.h>
#include<assert.h>
#define    KRNLADDR    PAGE_OFFSET

#ifndef    ASM
#include "types.h"
#include "printk.h"

void    *kmalloc(size_t size);
void    kfree(void *p);


static inline void *get_virt_pages(unsigned int n)
{
    assert(n>0);
    size_t    size = n << PAGE_SHIFT;
    return (void*) kmalloc(size);
}
static inline void free_virt_pages(void *p)
{
    kfree((void *)p);
}
static inline void *get_phys_pages(unsigned int n)
{
/*
    assert(n>0);
    size_t    size = n << PAGE_SHIFT;
    return (void*) va2pa(kmalloc(size));
*/
    return (void *)va2pa(get_virt_pages(n));
}
static inline void free_phys_pages(void *p)
{
    free_virt_pages((void*)va2pa(p));
}

extern inline void panic(char *msg);

extern char etext, edata, end;

char gdtr[6],idtr[6];
#define lgdt()    __asm__    __volatile__("lgdt gdtr")
#define    sgdt()    __asm__    __volatile__("sgdt gdtr")
#define    lidt()    __asm__    __volatile__("lidt idtr")
#define    sidt()    __asm__    __volatile__("sidt idtr")

#define    cli()    __asm__    __volatile__("cli")
#define    sti()    __asm__    __volatile__("sti")
#define    disableIRQ()    cli()
#define    enableIRQ()    sti()

#define    ALIGN(x, a)    (((x)+(a)-1) & ~((a)-1))


// 1 GB
#define    MAX_SUPT_PHYMM_SIZE    (1UL<<30)

#define    INT_STACK_SIZE    PAGE_SIZE


enum GDTSelectorIndex
{
    INDEX_SPACE=0,
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
// pushad push eax, ecx, edx, ebx, esp, ebp, esi, edi
typedef    struct
{
    u32    ebx;
    u32    edx;
    u32    ecx;
    u32    edi;
    u32    esi;
    u32    ebp;
    u32    eax;    // 因为在系统调用中用来带调用号，就无法传送参数
            // 所以把eax放在这个位置
    u16    ds, _ds;
    u16    es, _es;
    u16    fs, _fs;
    u16    gs, _gs;
    union
    {
        u32    sysc_nr;
        u32    irq;
        u32    errcode;
    };
    u32    eip;
    u16    cs, _cs;
    u32    eflags;
    u32    esp;
    u16    ss, _ss;
} PtRegs, *pPtRegs;

typedef    unsigned long    Dev, *pDev;

typedef struct system
{
    u32    mmap_addr;
    u32    mmap_size;    // Byte

    u32    mm_lower;    // KB
    u32    mm_upper;    // KB
    u64    mm_size;    // Byte

    u32    page_count;
    pPage    page_map;
    u32    page_bitmap;

    u32 *page_dir;
    u32    *pte_start;
    u32    *pte_end;

    u32    kernel_end;

    // +-------+-------+-------+-------+
    // | drive | part1 | part2 | part3 |
    // +-------+-------+-------+-------+
    // Partition numbers always start from zero.
    // Unused partition bytes must be set to 0xFF.
    // More Infomation see 'info multiboot'
    u32    boot_device;

    Dev    root_dev;
#define    CMD_LINE_SIZE    128
    char    *cmdline;

    u32    debug;
} System, *pSystem;

extern    System system;

#define    pgmap system.page_map

#endif


#define    SAVE_REGS   \
    cld;            \
    pushl    %gs;    \
    pushl    %fs;    \
    pushl    %es;    \
    pushl    %ds;    \
    pushl    %eax;    \
    pushl    %ebp;    \
    pushl    %esi;    \
    pushl    %edi;    \
    pushl    %ecx;    \
    pushl    %edx;    \
    pushl    %ebx;

#define    RESTORE_REGS    \
    popl    %ebx;    \
    popl    %edx;    \
    popl    %ecx;    \
    popl    %edi;    \
    popl    %esi;    \
    popl    %ebp;    \
    popl    %eax;    \
    popl    %ds;    \
    popl    %es;    \
    popl    %fs;    \
    popl    %gs;


#define    PRIVILEGE_KRNL    0x0
#define    PRIVILEGE_USER    0x3

#define    INDEX_UCODE    3
#define    INDEX_UDATA    4
/* *8 == <<3 .但要用于汇编文件 <<3 不行. */
#define    SELECTOR_KRNL_CS    (INDEX_KCODE*8)
#define    SELECTOR_KRNL_DS    (INDEX_KDATA*8)
#define    SELECTOR_KRNL_SS    SELECTOR_KRNL_DS
#define    SELECTOR_USER_CS    ((INDEX_UCODE*8)|PRIVILEGE_USER)
#define    SELECTOR_USER_DS    ((INDEX_UDATA*8)|PRIVILEGE_USER)
#define    SELECTOR_USER_SS    SELECTOR_USER_DS


#if 0
#define    INT_VECT_DIVIDE          0x0
#define    INT_VECT_DEBUG           0x1
#define    INT_VECT_NMI             0x2
#define    INT_VECT_BREAKPOINT      0x3    /* Break Point */
#define    INT_VECT_OVERFLOW        0x4
#define    INT_VECT_BOUNDS          0x5
#define    INT_VECT_INVALOP         0x6
#define    INT_VECT_COPROCNOT       0x7
#define    INT_VECT_DOUBLEFAULT     0x8    /* Double Fault */
#define    INT_VECT_COPROCSEG       0x9
#define    INT_VECT_INVALTSS        0xA
#define    INT_VECT_SEGNOT          0xB
#define    INT_VECT_STACKFAULT      0xC    /* Stack Fault */
#define    INT_VECT_PROTECTION      0xD
#define    INT_VECT_PAGEFAULT       0xE
#define    INT_VECT_COPROCERR       0x10
#endif
#define    INT_VECT_IRQ0    0x20
#define    INT_VECT_IRQ8    0x28

#define    REBOOT_RESTART   0x00
#define    REBOOT_POWEROFF  0x01


#define    ROOT_DEV    system.root_dev

#endif //_SYSTEM_H

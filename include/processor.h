/*
 *--------------------------------------------------------------------------
 *   File Name: processor.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Sun Aug 03 21:37:34 2008
 * Last Update: Sun Aug 03 21:37:34 2008
 *
 *--------------------------------------------------------------------------
 */

#ifndef _DESCRIPTOR_H
#define _DESCRIPTOR_H

#include "string.h"
#include "system.h"
#include "types.h"

// Descriptor Attrib.
#define DA_E 0x8   // Executable
#define DA_ED 0x4  // For Data SEG
#define DA_C 0x4   // For Code SEG
#define DA_W 0x2   // For Data SEG
#define DA_A 0x1   // Access. 1-Accessed.
// we just need Accessed.
// For Data Segment.
#define DSA_R (DA_A)
#define DSA_RW (DA_A | DA_W)
#define DSA_RD (DA_A | DA_ED)  // down to ...
#define DSA_RWD (DA_A | DA_W | DA_ED)
// For Code Segment.
#define CSA_E (DA_E | DA_A)
#define CSA_ER (DA_E | DA_A | DA_R)
#define CSA_EC (DA_E | DA_A | DA_C)
#define CSA_ERC (DA_E | DA_A | DA_R | DA_C)

//-------------------------------------------------------------------------
// Code And Data Descriptor.
// Program Segment Descriptor.
//-------------------------------------------------------------------------
typedef struct {
    unsigned short limitL;

    unsigned short baseL;

    unsigned char baseM;
    unsigned char type : 4;
    unsigned char S : 1;  // allways set to 0. NOT SYS SEG DESC
    unsigned char DPL : 2;
    unsigned char P : 1;  // allways set to 1.

    unsigned char limitH : 4;
    unsigned char AVL : 1;
    unsigned char zero : 1;  // should be set to 0
    unsigned char DB : 1;    // 0--16bits,1--32bits
    unsigned char G : 1;     // set to 1. We just need 4K size.
    unsigned char baseH;
} seg_t;

//-------------------------------------------------------------------------
// TSS State,LDT
// And Gates(Call,Interrupt,Trap,Tss) Descriptor.
//-------------------------------------------------------------------------
typedef struct {
    unsigned short eaddrL;

    unsigned short selector;

    unsigned char parmeter : 5;  // for call gate
                                 // reserved by other gates.
    unsigned char zero : 3;
    unsigned char type : 4;
    unsigned char S : 1;
    unsigned char DPL : 2;
    unsigned char P : 1;

    unsigned short eaddrH;

} gate_t;

// just used for type...
typedef union {
    seg_t seg;
    gate_t gate;
} desc_t;

#define NGDT 16
#define NIDT 256

extern desc_t idt[NIDT];
extern desc_t gdt[NGDT];

//-------------------------------------------------------------------------
// Define Gate Types...
//-------------------------------------------------------------------------
#define INTR_GATE 0x0E  // Clear 'IF' bit.---->Close Interrupt
#define TRAP_GATE 0x0F  // Keep  'IF' bit.
#define TSS_DESC 0x09

static inline void _init_desc(desc_t* desc) {
    memset((void*)desc, 0, sizeof(desc_t));
}

static inline desc_t _create_seg(u8 type, u8 DPL) {
    desc_t d;
    seg_t* p = &d.seg;

    _init_desc(&d);
    p->limitL = 0xFFFF;
    p->limitH = 0x0F;
    p->type = type;
    p->S = 0x1;
    p->DPL = DPL;
    p->P = 0x1;
    p->G = 0x1;
    p->DB = 0x1;

    return d;
}

static inline desc_t _create_gate(u32 handler, u8 type, u8 DPL) {
    desc_t d;
    gate_t* p = &d.gate;

    _init_desc(&d);

    p->eaddrL = 0xFFFF & handler;
    p->eaddrH = 0xFFFF & (handler >> 16);
    p->selector = SELECTOR_KRNL_CS;
    p->type = type;
    p->P = 0x1;
    p->DPL = DPL;

    return d;
}
static inline void set_idt_gate(u32 vec, u32 handler, u8 type, u8 DPL) {
    idt[vec] = _create_gate(handler, type, DPL);
}

#define set_sys_int(vect, type, DPL, handler)        \
    do {                                             \
        void handler();                              \
        set_idt_gate(vect, (u32)handler, type, DPL); \
    } while (0)

typedef struct tss {
    u16 backlink, _backlink;
    u32 esp0;
    u16 ss0, _ss0;
    u32 esp1;
    u16 ss1, _ss1;
    u32 esp2;
    u16 ss2, _ss2;
    u32 cr3;
    u32 eip;
    u32 eflags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;
    u16 es, _es;
    u16 cs, _cs;
    u16 ss, _ss;
    u16 ds, _ds;
    u16 fs, _fs;
    u16 gs, _gs;
    u16 ldt, _ldt;
    u16 T : 1, _T : 15, iomap_base;
} tss_t;

extern tss_t tss;

// CR0 含有控制处理器操作模式和状态的系统控制标志
#define CR0_PE (1 << 0)   // R/W Protection Enabled
#define CR0_MP (1 << 1)   // R/W Monitor Coprocessor
#define CR0_EM (1 << 2)   // R/W Emulation
#define CR0_TS (1 << 3)   // R/W Task Switched
#define CR0_ET (1 << 4)   // R   Extension Type
#define CR0_NE (1 << 5)   // R/W Numeric Error
#define CR0_WP (1 << 16)  // R/W Write Protect
#define CR0_AM (1 << 18)  // R/W Alignment Mask
#define CR0_NW (1 << 29)  // R/W Not Writethough
#define CR0_CD (1 << 30)  // R/W Cache Disable
#define CR0_PG (1 << 31)  // R/W Paging

// CR1 保留不用

// CR2 含有导致页错误的线性地址

// CR3 为页目录表的物理内存基地址，因此该寄存器也叫页目录基地址寄存器PDBR (Page-Directory Baseaddress Register)
// 如果不启用PAE:
//     CR3的12~31bit为 Page-Directory-Table Base Address，有两层页表，最高页目录大小为2^12=4096=4KB
//     每个页目录项为4Byte，所以共1024项。共能寻址4GB物理内存
// 如果启用PAE：
//     CR3的 5~31bit为 Page-Directory-Table Base Address，有三层页表，最高页目录大小为2^5=32Byte
//     每个页目录项为8Byte，所以共4项，能寻址64GB物理内存

#define CR3_PWT (1 << 3)  // PWT Page-Level Writethrough
#define CR3_PCD (1 << 4)  // PCD Page-Level Cache Disable, 0: 表示最高目录表可缓存；1: 表示不可缓存

// CR4
#define CR4_VME (1 << 0)           // R/W Virtual-8086 Mode Extensions
#define CR4_PVI (1 << 1)           // R/W Protected-Mode Virtual Interrupts
#define CR4_TSD (1 << 2)           // R/W Time Stamp Disable
#define CR4_DE (1 << 3)            // R/W Debugging Extensions
#define CR4_PSE (1 << 4)           // R/W Page Size Extensions
#define CR4_PAE (1 << 5)           // R/W Physical-Address Extension; 0: 不启用; 1: 启用，支持2MB的超级页
#define CR4_MCE (1 << 6)           // R/W Machine Check Enable
#define CR4_PGE (1 << 7)           // R/W Page-Global Enable
#define CR4_PCE (1 << 8)           // R/W Performance-Monitoring Counter Enable
#define CR4_OSFXSR (1 << 9)        // R/W Operating System FXSAVE/FXRSTOR Support
#define CR4_OSXMMEEXCPT (1 << 10)  // R/W  Operating System Unmasked Exception Support
#define CR4_OSXSAVE (1 << 18)      // R/W XSAVE and Processor Extended States Enable Bit

#endif  //_DESCRIPTOR_H

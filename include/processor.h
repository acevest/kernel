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
} Seg, *pSeg;

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

} Gate, *pGate;

// just used for type...
typedef union {
    Seg seg;
    Gate gate;
} Desc, *pDesc;

#define NGDT 256
#define NIDT 256
#define NLDT 5
extern Desc idt[NIDT];
extern Desc gdt[NGDT];

//-------------------------------------------------------------------------
// Define Gate Types...
//-------------------------------------------------------------------------
#define INTR_GATE 0x0E  // Clear 'IF' bit.---->Close Interrupt
#define TRAP_GATE 0x0F  // Keep  'IF' bit.
#define TSS_DESC 0x09

static inline void _init_desc(pDesc desc) { memset((char *)desc, 0, sizeof(Desc)); }

static inline Desc _create_seg(u8 type, u8 DPL) {
    Desc d;
    pSeg p = &d.seg;

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

static inline Desc _create_gate(u32 handler, u8 type, u8 DPL) {
    Desc d;
    pGate p = &d.gate;

    _init_desc(&d);

    p->eaddrL = 0xFFFF & handler;
    p->eaddrH = 0xFFFF & (handler >> 16);
    p->selector = SELECTOR_KRNL_CS;
    p->type = type;
    p->P = 0x1;
    p->DPL = DPL;

    return d;
}
static inline void set_idt_gate(u32 vec, u32 handler, u8 type, u8 DPL) { idt[vec] = _create_gate(handler, type, DPL); }

#define set_sys_int(vect, type, DPL, handler)        \
    do {                                             \
        void handler();                              \
        set_idt_gate(vect, (u32)handler, type, DPL); \
    } while (0)

typedef struct {
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
} TSS, *pTSS;
static inline void set_tss_gate(u32 vec, u32 addr) {
#if 1
    pSeg p = (pSeg)(gdt + vec);
    _init_desc((pDesc)p);
    p->limitL = 0xFFFF & sizeof(TSS);
    p->limitH = 0x0F & (sizeof(TSS) >> 16);
    p->baseL = 0xFFFF & addr;
    p->baseM = 0xFF & (addr >> 16);
    p->baseH = 0xFF & (addr >> 24);

    p->P = 1;
    p->DPL = PRIVILEGE_USER;
    p->S = 0;

    p->type = TSS_DESC;
#endif
}

extern TSS tss;

#endif  //_DESCRIPTOR_H

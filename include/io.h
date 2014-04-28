/*
 *--------------------------------------------------------------------------
 *   File Name: io.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Mon Jun 18 23:25:23 2007
 * Last Update: Mon Jun 18 23:25:23 2007
 * 
 *--------------------------------------------------------------------------
 */

#ifndef    _IO_H
#define _IO_H

#include <types.h>

#define outb_p(value,port)({                    \
__asm__("outb    %%al,%%dx;nop;nop;nop;nop"     \
:                                               \
:"a" (value),"d" (port));                       \
})

#define inb_p(port)({                           \
u8 _bt;                                         \
__asm__("inb    %%dx,%%al;nop;nop;nop;nop"      \
:"=a" (_bt)                                     \
:"d" (port));                                   \
_bt;                                            \
})


#define outb(value,port)({      \
__asm__("outb    %%al,%%dx"     \
:                               \
:"a" (value),"d" (port));       \
})

#define outw(value,port)({      \
__asm__("outw    %%ax,%%dx"     \
:                               \
:"a" (value),"d" (port));       \
})
#define outl(value,port)({      \
__asm__("outl    %%eax,%%dx"    \
:                               \
:"a" (value),"d" (port));       \
})

#define inb(port)({             \
u8 _bt;                         \
asm("inb    %%dx,%%al"          \
:"=a" (_bt)                     \
:"d" (port));                   \
_bt;                            \
})

#define inw(port)({             \
u16 _bt;                        \
asm("inw    %%dx,%%ax"          \
:"=a" (_bt)                     \
:"d" (port));                   \
_bt;                            \
})

#define inl(port)({             \
u16 _bt;                        \
asm("inl    %%dx,%%eax"         \
:"=a" (_bt)                     \
:"d" (port));                   \
_bt;                            \
})


#define BUILDIO(bwl, type)                                              \
static inline void ins##bwl(int port, void *buf, unsigned long count)   \
{                                                                       \
    asm volatile(    "cld;rep;ins" #bwl                                 \
            : "+c"(count), "+D"(buf) : "d"(port));                      \
}


BUILDIO(b, char)
BUILDIO(w, short)
BUILDIO(l, int)

#endif //_IO_H

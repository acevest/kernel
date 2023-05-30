/*
 *--------------------------------------------------------------------------
 *   File Name: entry.S
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Thu Jul 09 19:56:40 2009
 * Last Update: Thu Jul 09 19:56:40 2009
 *
 *--------------------------------------------------------------------------
 */
#define ASM
#include <system.h>
#include <linkage.h>

/*
 * 陷阱门不会自动清除IF位.所以必需先关中断.
 * 这个bug我调了很久很久......
 */
#define EC_ENTRY(name)      \
    .extern    do##name;       \
    ENTRY(name)             \
    cli; /* 非常重要 */

#define EC_CALLDO(name)     \
    SAVE_REGS                \
    movw    %ss, %ax;    \
    movw    %ax, %ds;    \
    movw    %ax, %es;    \
    movw    %ax, %fs;    \
    movw    %ax, %gs;    \
    call    do##name;        \
    RESTORE_REGS;            \
    addl    $4,%esp;        \
    iret;


/*
 * 不会自动压入错误码的,调用此宏压入一个
 * 零来占据错误码的位置.以保持代码风格的
 * 一致性.
 */
#define NOERRCODE(name)     \
    EC_ENTRY(name)          \
    pushl    $0;             \
    EC_CALLDO(name)

#define ERRORCODE(name)     \
    EC_ENTRY(name)          \
    EC_CALLDO(name)


NOERRCODE    (DivideError)
NOERRCODE    (Debug)
NOERRCODE    (NMI)
NOERRCODE    (BreakPoint)
NOERRCODE    (OverFlow)
NOERRCODE    (BoundsCheck)
NOERRCODE    (InvalidOpcode)
NOERRCODE    (DeviceNotAvailable)
NOERRCODE    (DoubleFault)
NOERRCODE    (CoprocSegOverRun)
ERRORCODE    (InvalidTss)
ERRORCODE    (SegNotPresent)
ERRORCODE    (StackFault)
ERRORCODE    (GeneralProtection)
ERRORCODE    (PageFault)
NOERRCODE    (CoprocError)

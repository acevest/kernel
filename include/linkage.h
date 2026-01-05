/*
 *--------------------------------------------------------------------------
 *   File Name: linkage.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Fri Jul  10 11:19:26 2009
 * Last Update: Fri Jul  10 11:19:26 2009
 *
 *--------------------------------------------------------------------------
 */
#ifndef __LINKAGE_H
#define __LINKAGE_H

#ifdef ASM

#define ALIGN .align 0x04, 0x90
#define ALIGN_STR ".align    0x04,0x90"
#define ENTRY(symbol) \
    .global symbol;   \
    ALIGN;            \
    symbol:

#endif

#endif

#define __initdata __attribute__((__section__(".init.data")))

#define ALIGN8 __attribute__((__aligned__(8)))
#define ALIGN4 __attribute__((__aligned__(4)))

/*
 *--------------------------------------------------------------------------
 *   File Name: types.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Wed Aug 13 23:06:22 2008
 * Last Update: Wed Aug 13 23:06:22 2008
 * 
 *--------------------------------------------------------------------------
 */

#ifndef _TYPES_H
#define _TYPES_H

typedef unsigned int        size_t;
typedef int                 ssize_t;

typedef signed char         s8;
typedef signed short        s16;
typedef signed long         s32;
typedef signed long long    s64;

typedef unsigned char       u8;
typedef unsigned short      u16;
typedef unsigned long       u32;
typedef unsigned long       u32;
typedef unsigned long long  u64;

typedef signed char         s8_t;
typedef signed short        s16_t;
typedef signed long         s32_t;
typedef signed long long    s64_t;

typedef unsigned char       u8_t;
typedef unsigned short      u16_t;
typedef unsigned long       u32_t;
typedef unsigned long       u32_t;
typedef unsigned long long  u64_t;

typedef unsigned long       pid_t;
typedef unsigned long       mode_t;


#define NULL ((void*)0)

typedef enum { false, true } bool;

//=========================================================================
//Define kinds of function's type ...
//=========================================================================
typedef    void    (*pf_intr)();

#endif //_TYPES_H

/*
 *--------------------------------------------------------------------------
 *   File Name: msr.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Fri Jan  2 19:54:18 2009
 * Last Update: Fri Jan  2 19:54:18 2009
 * 
 *--------------------------------------------------------------------------
 */

#ifndef    _MSR_H
#define _MSR_H

#define MSR_SYSENTER_CS        0x174
#define MSR_SYSENTER_ESP    0x175
#define MSR_SYSENTER_EIP    0x176

#define wrmsr(msr, lowval, highval) do{\
    asm("wrmsr;"::"c"(msr),"a"(lowval),"d"(highval));\
}while(0);


#endif //_MSR_H

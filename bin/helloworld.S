/*
 *--------------------------------------------------------------------------
 *   File Name: helloworld.S
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Tue Feb 23 20:15:27 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#define ASM
#include <syscall.h>
.global    _start
_start:
    nop
    nop
    nop
    #xchg %bx, %bx
loop:
    #jmp    loop
    call    hw
    
    nop
    nop
    nop
    nop
    nop
die:
    jmp    die

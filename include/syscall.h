/*
 *--------------------------------------------------------------------------
 *   File Name: syscall.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Fri Jan  2 19:52:28 2009
 * Last Update: Tue Feb 23 02:32:35 2010
 * 
 *--------------------------------------------------------------------------
 */

#ifndef    _SYSCALL_H
#define _SYSCALL_H

#define SYSC_NUM    256

#ifndef    ASM

#include "page.h"
#include "errno.h"
typedef    int    SyscReturn;
typedef SyscReturn  (*pfSysc)();


#define SYSENTER        \
    asm(            \
    "pushl    %ecx;"        \
    "pushl    %edx;"        \
    "pushl    %ebp;"        \
    "pushl    $1f;"        \
    "movl    %esp,%ebp;"    \
    "sysenter;"        \
    "1:"            \
    "addl    $4,%esp;"    \
    "popl    %ebp;"        \
    "popl    %edx;"        \
    "popl    %ecx;");    \



#define sysenter(vect)({     \
    asm(""::"a"(vect));    \
    SYSENTER        \
})
#if 0
#define syscall0(vect)(({    \
    sysenter(vect);        \
}), ({int i;asm("":"=a"(i));i;}))
#endif
#define _syscall0(vect)({    \
    sysenter(vect);        \
})

#define _syscall1(vect, a)({    \
    asm(""::"b"(a));    \
    sysenter(vect);        \
})

#define _syscall2(vect, a, b)({    \
    asm(""::"b"(a), "d"(b));\
    sysenter(vect);        \
})

#define _syscall3(vect, a, b, c)({    \
    asm(""::"b"(a), "d"(b), "c"(c));\
    sysenter(vect);            \
})

#define _syscall_ret()({    \
    int ret;        \
    asm("":"=a"(ret));    \
    if(ret < 0)        \
    {            \
        errno = -ret;    \
        ret = -1;    \
    }            \
    ret;})

#define syscall0(vect)        \
    (({_syscall0(vect);}),        ({_syscall_ret();}))
#define syscall1(vect, a)    \
    (({_syscall1(vect, a);}),    ({_syscall_ret();}))
#define syscall2(vect, a, b)    \
    (({_syscall2(vect, a, b);}),    ({_syscall_ret();}))
#define syscall3(vect, a, b, c)    \
    (({_syscall3(vect, a, b, c);}),    ({_syscall_ret();}))
#if 1
enum
{
    SYSC_WRITE,
    SYSC_READ_KBD,
    SYSC_REBOOT,
    SYSC_FORK,
    SYSC_EXEC,
    SYSC_OPEN,
    SYSC_READ,
    SYSC_STAT,
    SYSC_EXIT,
    SYSC_PAUSE,
    SYSC_TEST
};
#endif

#endif    // ASM

#endif //_SYSCALL_H

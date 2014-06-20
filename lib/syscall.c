/*
 * ------------------------------------------------------------------------
 *   File Name: syscall.c
 *      Author: Zhao Yanbai
 *              Fri Jun 20 22:57:17 2014
 * Description: none
 * ------------------------------------------------------------------------
 */


#define SYSENTER_ASM            \
        "pushl  $1f;"           \
        "pushl    %%ecx;"       \
        "pushl    %%edx;"       \
        "pushl    %%ebp;"       \
        "movl     %%esp,%%ebp;" \
        "sysenter;"             \
        "1:"

static int __syscall0(int nr)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr));
    return __sysc_ret__;
}

static int __syscall1(int nr, unsigned long a)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr), "b"(a));
    return __sysc_ret__;
}

static int __syscall2(int nr, unsigned long a, unsigned long b)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr), "b"(a), "c"(b));
    return __sysc_ret__;
}

static int __syscall3(int nr, unsigned long a, unsigned long b, unsigned long c)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr), "b"(a), "c"(b), "d"(c));
    return __sysc_ret__;
}

static int __syscall4(int nr, unsigned long a, unsigned long b, unsigned long c, unsigned long d)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr), "b"(a), "c"(b), "d"(c), "S"(d));
    return __sysc_ret__;
}

static int __syscall5(int nr, unsigned long a, unsigned long b, unsigned long c, unsigned long d, unsigned long e)
{
    int __sysc_ret__ = 0;
    asm(SYSENTER_ASM:"=a"(__sysc_ret__):"a"(nr), "b"(a), "c"(b), "d"(c), "S"(d), "D"(e));
    return __sysc_ret__;
}



int _syscall0(int nr)
{
    return __syscall0(nr);
}

int _syscall1(int nr, unsigned long a)
{
    return __syscall1(nr, a);
}

int _syscall2(int nr, unsigned long a, unsigned long b)
{
    return __syscall2(nr, a, b);
}

int _syscall3(int nr, unsigned long a, unsigned long b, unsigned long c)
{
    return __syscall3(nr, a, b, c);
}

int _syscall4(int nr, unsigned long a, unsigned long b, unsigned long c, unsigned long d)
{
    return __syscall4(nr, a, b, c, d);
}

int _syscall5(int nr, unsigned long a, unsigned long b, unsigned long c, unsigned long d, unsigned long e)
{
    return __syscall5(nr, a, b, c, d, e);
}

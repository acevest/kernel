/*
 *--------------------------------------------------------------------------
 *   File Name: bits.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Wed Mar  4 20:56:29 2009
 * Last Update: Wed Mar  4 20:56:29 2009
 *
 *--------------------------------------------------------------------------
 */

#ifndef _BITS_H
#define _BITS_H

//#define    SET_BIT(bit) (1UL<<bit)
//#define    CLR_BIT(bit) (~(1UL<<bit))
//#define    ISSET_BIT(val,bit) ((val) & SET_BIT(bit))
#define SET_BIT(val, bit) (val |= (1UL << bit))
#define CLR_BIT(val, bit) (val &= (~(1UL << bit)))
#define XOR_BIT(val, bit) (btc((unsigned int *)&val, bit), val)
#define ISSET_BIT(val, bit) (val & (1UL << bit))
#define ISCLR_BIT(val, bit) (!ISSET_BIT(val, bit))

#define BITS_PER_LONG (sizeof(unsigned long) * 8)

static inline void btc(unsigned int *v, unsigned int b) { asm("btc %1,%0" : "=m"(*v) : "Ir"(b)); }

static inline int test_and_set_bit(long nr, volatile unsigned long *addr) {
    int oldbit;

    asm("bts %2,%1\n\t"
        "sbb %0,%0"
        : "=r"(oldbit), "+m"(*(volatile long *)(addr))
        : "Ir"(nr));
    return oldbit;
}

static inline int test_and_clear_bit(int nr, volatile unsigned long *addr) {
    int oldbit;

    asm volatile(
        "btr %2,%1\n\t"
        "sbb %0,%0"
        : "=r"(oldbit), "+m"(*(volatile long *)(addr))
        : "Ir"(nr)
        : "memory");

    return oldbit;
}
/**
 * test_and_change_bit - Change a bit and return its old value
 * @nr: Bit to change
 * @addr: Address to count from
 *
 * This operation is atomic and cannot be reordered.
 * It also implies a memory barrier.
 */
static inline int test_and_change_bit(int nr, volatile unsigned long *addr) {
    int oldbit;

    asm volatile(
        "btc %2,%1\n\t"
        "sbb %0,%0"
        : "=r"(oldbit), "+m"(*(volatile long *)(addr))
        : "Ir"(nr)
        : "memory");

    return oldbit;
}

/**
 * change_bit - Toggle a bit in memory
 * @nr: the bit to change
 * @addr: the address to start counting from
 *
 * Unlike change_bit(), this function is non-atomic and may be reordered.
 * If it's called on the same region of memory simultaneously, the effect
 * may be that only one operation succeeds.
 */
static inline void change_bit(int nr, volatile unsigned long *addr) {
    asm volatile("btc %1,%0" : "+m"(*(volatile long *)(addr)) : "Ir"(nr));
}

static inline int constant_test_bit(unsigned int nr, const volatile unsigned long *addr) {
    return ((1UL << (nr % BITS_PER_LONG)) & (((unsigned long *)addr)[nr / BITS_PER_LONG])) != 0;
}

/**
 * find_first_bit - find the first set bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum size to search
 *
 * Returns the bit-number of the first set bit, not the number of the byte
 * containing a bit.
 */
static inline int find_first_bit(const unsigned long *addr, unsigned size) {
    int d0, d1;
    int res;

    /* This looks at memory. Mark it volatile to tell gcc not to move it around */
    __asm__ __volatile__(
        "xorl %%eax,%%eax\n\t"
        "repe; scasl\n\t"
        "jz 1f\n\t"
        "leal -4(%%edi),%%edi\n\t"
        "bsfl (%%edi),%%eax\n"
        "1:\tsubl %%ebx,%%edi\n\t"
        "shll $3,%%edi\n\t"
        "addl %%edi,%%eax"
        : "=a"(res), "=&c"(d0), "=&D"(d1)
        : "1"((size + 31) >> 5), "2"(addr), "b"(addr)
        : "memory");
    return res;
}

/**
 * find_first_zero_bit - find the first zero bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum size to search
 *
 * Returns the bit-number of the first zero bit, not the number of the byte
 * containing a bit.
 */
static inline int find_first_zero_bit(const unsigned long *addr, unsigned size) {
    int d0, d1, d2;
    int res;

    if (!size) return 0;
    /* This looks at memory. Mark it volatile to tell gcc not to move it around */
    __asm__ __volatile__(
        "movl $-1,%%eax\n\t"
        "xorl %%edx,%%edx\n\t"
        "repe; scasl\n\t"
        "je 1f\n\t"
        "xorl -4(%%edi),%%eax\n\t"
        "subl $4,%%edi\n\t"
        "bsfl %%eax,%%edx\n"
        "1:\tsubl %%ebx,%%edi\n\t"
        "shll $3,%%edi\n\t"
        "addl %%edi,%%edx"
        : "=d"(res), "=&c"(d0), "=&D"(d1), "=&a"(d2)
        : "1"((size + 31) >> 5), "2"(addr), "b"(addr)
        : "memory");
    return res;
}

/**
 * find_next_zero_bit - find the first zero bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The maximum size to search
 */
static inline int find_next_zero_bit(const unsigned long *addr, int size, int offset) {
    unsigned long *p = ((unsigned long *)addr) + (offset >> 5);
    int set = 0, bit = offset & 31, res;

    if (bit) {
        /*
         * Look for zero in the first 32 bits.
         */
        __asm__(
            "bsfl %1,%0\n\t"
            "jne 1f\n\t"
            "movl $32, %0\n"
            "1:"
            : "=r"(set)
            : "r"(~(*p >> bit)));
        if (set < (32 - bit)) return set + offset;
        set = 32 - bit;
        p++;
    }
    /*
     * No zero yet, search remaining full bytes for a zero
     */
    res = find_first_zero_bit(p, size - 32 * (p - (unsigned long *)addr));
    return (offset + set + res);
}

static inline int variable_test_bit(int nr, volatile const unsigned long *addr) {
    int oldbit;

    asm volatile(
        "bt %2,%1\n\t"
        "sbb %0,%0"
        : "=r"(oldbit)
        : "m"(*(unsigned long *)addr), "Ir"(nr));

    return oldbit;
}
#endif  //_BITS_H

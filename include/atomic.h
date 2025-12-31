/*
 * ------------------------------------------------------------------------
 *   File Name: atomic.h
 *      Author: Zhao Yanbai
 *              Sat Jun 21 18:37:21 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <types.h>

#if 1

typedef struct atomic {
    volatile int counter;
} atomic_t;

static inline void atomic_inc(atomic_t* v) {
    __sync_add_and_fetch(&(v->counter), 1);
}
static inline void atomic_dec(atomic_t* v) {
    __sync_sub_and_fetch(&(v->counter), 1);
}

static inline int atomic_read(atomic_t* v) {
    return *((int*)(&(v->counter)));
}

static inline void atomic_set(atomic_t* v, int i) {
    __sync_lock_test_and_set(&(v->counter), i);
}

#else

// 以下的定义方式
// 优点：
//  1. 简化了类型定义，代码更简洁
//  2. 可能在某些情况下减少内存开锁
// 缺点：
//  1. 会丢失volatile限定符。volatile对于多线程和中断处理等场景中很重要。
//  2. 限制了扩展性
typedef uint32_t atomic_t;

#define atomic_inc(x) __sync_add_and_fetch((x), 1)
#define atomic_dec(x) __sync_sub_and_fetch((x), 1)
#define atomic_add(x, y) __sync_add_and_fetch((x), (y))
#define atomic_sub(x, y) __sync_sub_and_fetch((x), (y))
#endif

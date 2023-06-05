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

typedef uint32_t atomic_t;

#define atomic_inc(x) __sync_add_and_fetch((x), 1)
#define atomic_dec(x) __sync_sub_and_fetch((x), 1)
#define atomic_add(x, y) __sync_add_and_fetch((x), (y))
#define atomic_sub(x, y) __sync_sub_and_fetch((x), (y))

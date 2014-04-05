/*
 * ------------------------------------------------------------------------
 *   File Name: bug.h
 *      Author: Zhao Yanbai
 *              Sat Apr  5 14:51:50 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#define BUG() do {                                          \
    printk("kernel BUG at %s:%d!\n", __FILE__, __LINE__);   \
    panic("BUG!");                                          \
} while (0)

#define BUG_ON(condition) do { if (unlikely((condition)!=0)) BUG(); } while(0)

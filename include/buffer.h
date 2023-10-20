/*
 * ------------------------------------------------------------------------
 *   File Name: buffer.h
 *      Author: Zhao Yanbai
 *              2023-10-11 23:47:15 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <atomic.h>
#include <fs.h>
#include <mm.h>
#include <page.h>
#include <system.h>
#include <wait.h>

typedef struct bbuffer {
    uint32_t block;  // block number
    void *data;      //
    atomic_t ref_count;
    dev_t dev;
    page_t *page;
    list_head_t node;
    wait_queue_head_t waitq_lock;
    uint16_t block_size;  // block size
    uint16_t uptodate : 1;
    uint16_t locked : 1;
    uint16_t dirt : 1;  // 还不支持
} bbuffer_t;

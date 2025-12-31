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
#include <completion.h>
#include <fs.h>
#include <mm.h>
#include <page.h>
#include <system.h>
typedef struct bbuffer {
    uint32_t block;  // block number
    void* data;      //
    atomic_t ref_count;
    dev_t dev;
    page_t* page;
    list_head_t node;
    completion_t io_done;
    uint16_t block_size;  // block size
    uint16_t uptodate : 1;
} bbuffer_t;

bbuffer_t* bread(dev_t dev, uint64_t block, uint32_t size);

void brelse(bbuffer_t* b);

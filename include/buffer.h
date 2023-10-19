/*
 * ------------------------------------------------------------------------
 *   File Name: buffer.h
 *      Author: Zhao Yanbai
 *              2023-10-11 23:47:15 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <fs.h>
#include <mm.h>
#include <page.h>
#include <system.h>

typedef struct bbuffer {
    uint32_t block;  // block number
    char *data;      //
    uint32_t ref_count;
    dev_t dev;
    page_t *page;
    list_head_t node;
    uint16_t block_size;  // block size
} bbuffer_t;

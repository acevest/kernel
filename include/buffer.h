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

typedef struct bbuffer {
    uint32_t block;       // block number
    char *data;           //
    uint16_t block_size;  // block size
    dev_t dev;
    page_t *page;
    struct bbuffer *next;
    struct bbuffer *hash_next;
} bbuffer_t;

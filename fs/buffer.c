/*
 * ------------------------------------------------------------------------
 *   File Name: buffer.c
 *      Author: Zhao Yanbai
 *              2023-06-20 19:30:33 Tuesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <buffer.h>

#define BLOCK_BUFFER_HASH_TABLE_SIZE 211
// atomic_t hash_cnt;
bbuffer_t *block_buffer_hash_table[BLOCK_BUFFER_HASH_TABLE_SIZE] = {
    0,
};

int hash(dev_t dev, uint32_t block) { return ((~dev) ^ block) % BLOCK_BUFFER_HASH_TABLE_SIZE; }

bbuffer_t *get_hash_block_buffer(dev_t dev, uint32_t block, uint16_t size) {}

void init_buffer() {}

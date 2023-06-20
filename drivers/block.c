/*
 * ------------------------------------------------------------------------
 *   File Name: block.c
 *      Author: Zhao Yanbai
 *              2023-06-20 19:30:47 Tuesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <disk.h>
#include <fs.h>
#include <ide.h>

blk_buffer_t *block_read(dev_t dev, uint32_t block, uint32_t size) {
    blk_buffer_t *bb = 0;

    assert(DEV_MAJOR(dev) == DEV_MAJOR_IDE0);
    assert(DEV_MINOR(dev) == 1);

    // 目前不提供hash表组强起来的缓冲功能
    // 直接读

    ide_disk_read(dev, block, size, bb->data);

    return bb;
}

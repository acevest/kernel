/*
 * ------------------------------------------------------------------------
 *   File Name: ext2.c
 *      Author: Zhao Yanbai
 *              Sun Jul  6 13:23:05 2014
 * Description: none
 * ------------------------------------------------------------------------
 */
#include "system.h"
#include "fs.h"
#include "ext2.h"

struct {
    ext2_sb_t ext2_sb;
} ext2_fs;

extern void blk_rw(dev_t dev, u64_t offset, u32_t scnt, char *buf);

#define BLKRW(offset, blkcnt, buf) do { blk_rw(system.root_dev, offset, (blkcnt)*EXT2_BLOCK_SIZE, buf); } while(0)

void ext2_setup_fs()
{
    memset(&ext2_fs, 0, sizeof(ext2_fs));

    char *buf = kmalloc(EXT2_BLOCK_SIZE, 0);
    if(buf == 0)
        panic("out of memory");

    printk("EXT2_BLOCK_SIZE %u\n", EXT2_BLOCK_SIZE);

    BLKRW(EXT2_SB_OFFSET, 1, buf);

    memcpy(EXT2_SB, buf, sizeof(*(EXT2_SB)));

    if(EXT2_SB->s_magic != EXT2_SUPER_MAGIC)
    {
        printk("file system magic %04x\n", EXT2_SB->s_magic);
        panic("only support ext2 file system...");
    }

    printk("Ext2 File System Information:\n");
    printk("inodes cnt %u blocks cnt %u free blocks %u free inodes %u\n",
        EXT2_SB->s_inodes_count, EXT2_SB->s_blocks_count, EXT2_SB->s_free_blocks_count, EXT2_SB->s_free_inodes_count);
    printk("block size %u log block size %u first data block %u\n",
        EXT2_BLOCK_SIZE, EXT2_SB->s_log_block_size, EXT2_SB->s_first_data_block);
    printk("blocks per group %u inodes per group %u\n", EXT2_SB->s_blocks_per_group, EXT2_SB->s_inodes_per_group);
}




void setup_fs()
{
    ext2_setup_fs();
}

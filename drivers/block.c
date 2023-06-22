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

#define BLOCK_BUFFER_HASH_TABLE_SIZE 37
atomic_t hash_cnt;
blk_buffer_t *block_buffer_hash_table[BLOCK_BUFFER_HASH_TABLE_SIZE] = {
    0,
};

int hash(dev_t dev, uint32_t block) { return ((co ~dev) ^ block) % BLOCK_BUFFER_HASH_TABLE_SIZE; }

blk_buffer_t *get_hash_block_buffer(dev_t dev, uint32_t block, uint16_t size) {}

blk_buffer_t *block_read(dev_t dev, uint32_t block) {
    blk_buffer_t *bb = 0;

    assert(DEV_MAJOR(dev) == DEV_MAJOR_IDE0);
    assert(DEV_MINOR(dev) == 1);

    // 目前不提供hash表组强起来的缓冲功能
    // 直接读

    // TODO:根据dev得到正确的blocksize
    const int blocksize = 1024;

    bb->data = kmalloc(blocksize, 0);  // debug

    ide_disk_read(dev, block * blocksize / 512, 1, bb->data);

    return bb;
}

#include <ext2.h>
// 读hda1 的 super block
void ata_read_ext2_sb() {
    // 初始默认blocksize = 1024
    // 则ext2_superblock应该在第1个block的offset为0的位置
    // ext2_superblock默认大小1024

    const int offset = 0;
    const int size = offset + 1024;
    const int block = 1;

    blk_buffer_t *bb = block_read(MAKE_DEV(DEV_MAJOR_IDE0, 1), block);

    ext2_sb_t *p = (ext2_sb_t *)(bb->data + offset);
    printk("inodes count %u inodes per group %u free %u\n", p->s_inodes_count, p->s_inodes_per_group,
           p->s_free_inodes_count);
    printk("blocks count %u blocks per group %u free %u magic %04x\n", p->s_blocks_count, p->s_blocks_per_group,
           p->s_free_blocks_count, p->s_magic);
    printk("first ino %u inode size %u first data block %u\n", p->s_first_ino, p->s_inode_size, p->s_first_data_block);
    printk("log block size %u write time %u\n", p->s_log_block_size, p->s_wtime);
    p->s_volume_name[63] = 0;
    printk("volume %s\n", p->s_volume_name);
    kfree(bb->data);
}

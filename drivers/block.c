/*
 * ------------------------------------------------------------------------
 *   File Name: block.c
 *      Author: Zhao Yanbai
 *              2023-06-20 19:30:47 Tuesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <buffer.h>
#include <disk.h>
#include <fs.h>
#include <ide.h>

void ide_disk_read(dev_t dev, uint32_t sect_nr, uint32_t count, bbuffer_t* b);
void block_read(bbuffer_t* b) {
    assert(b != NULL);
    assert(b->data != NULL);
    assert(b->page != NULL);
    assert(DEV_MAJOR(b->dev) == DEV_MAJOR_DISK);
    assert(b->block_size != 0);

    ide_disk_read(b->dev, (b->block * b->block_size) / 512, b->block_size / 512, b);
}

#include <ext2.h>
// 读hda1 的 super block
void ata_read_ext2_sb() {
    // 初始默认blocksize = 1024
    // 则ext2_superblock应该在第1个block的offset为0的位置
    // ext2_superblock默认大小1024

    const int block = 0;
    const int offset = 1024;
    const int size = 4096;
    bbuffer_t* bb = bread(system.root_dev, block, size);

    ext2_sb_t* p = (ext2_sb_t*)(bb->data + offset);
    printk("inodes count %u inodes per group %u free %u\n", p->s_inodes_count, p->s_inodes_per_group,
           p->s_free_inodes_count);
    printk("blocks count %u blocks per group %u free %u magic %04x\n", p->s_blocks_count, p->s_blocks_per_group,
           p->s_free_blocks_count, p->s_magic);
    printk("first ino %u inode size %u first data block %u\n", p->s_first_ino, p->s_inode_size, p->s_first_data_block);
    printk("log block size %u write time %u\n", p->s_log_block_size, p->s_wtime);
    char volume_name[16 + 1];
    strncpy(volume_name, p->s_volume_name, 16);
    volume_name[16] = 0;
    printk("volume %s\n", volume_name);
}

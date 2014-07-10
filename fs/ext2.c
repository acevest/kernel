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
#include "mm.h"
#include "ext2.h"

struct {
    ext2_sb_t ext2_sb;
    ext2_gd_t *ext2_gd;
} ext2_fs;

extern void blk_rw(dev_t dev, u64_t offset, u32_t scnt, char *buf);

#define BLKRW(blkid, blkcnt, buf) do { blk_rw(system.root_dev, 1ULL*(blkid)*EXT2_BLOCK_SIZE, (blkcnt)*EXT2_BLOCK_SIZE, buf); } while(0)

kmem_cache_t *ext2_block_cache;
kmem_cache_t *ext2_inode_cache;

ext2_inode_t ext2_root_inode;

static ext2_inode_t boot_inode;
static ext2_inode_t krnl_inode;

void *ext2_alloc_block()
{
    return (void *) kmem_cache_alloc(ext2_block_cache, 0);
}

void *ext2_free_block(void *blk)
{
    return;
    kmem_cache_free(ext2_block_cache, blk);
}

void *ext2_alloc_inode()
{
    return (void *) kmem_cache_alloc(ext2_inode_cache, 0);
}

#define ext2_gd(n) ((ext2_gd_t*)(EXT2_GD) + (n))
unsigned int sys_clock();
void ext2_read_inode(unsigned int ino, ext2_inode_t *inode)
{
    void *blk = ext2_alloc_block();
    assert(blk != 0);

    unsigned int in;    // inode number
    unsigned int gn;    // group number
    unsigned int gi;    // inode index in group

    gn = (ino-1) / EXT2_INODES_PER_GROUP;
    gi = (ino-1) % EXT2_INODES_PER_GROUP;

    unsigned int blkid = gi / EXT2_INODES_PER_BLOCK; // inode blkid
    unsigned int inoff = gi % EXT2_INODES_PER_BLOCK; // inode offset

    blkid += ext2_gd(gn)->bg_inode_table;
    inoff *= EXT2_INODE_SIZE;

    printd("group %u %u blkid %u blkoff %u clock %u\n", gn, gi, blkid, inoff, sys_clock());

    BLKRW(blkid, 1, blk);

    memcpy(inode, blk+inoff, sizeof(ext2_inode_t));

    ext2_free_block(blk);
}

unsigned int ext2_search_indir(const char *name, const ext2_inode_t *inode)
{
    unsigned int ino = 0;

    void *blk = ext2_alloc_block();
    assert(blk != 0);

    BLKRW(inode->i_block[0], 1, blk); // only support the first direct blocks

    ext2_dirent_t *dirent = (ext2_dirent_t *) blk;
    while(dirent->name_len != 0)
    {
        dirent->name[dirent->name_len] = 0;
        printk("  dirent %s inode %u rec_len %u name_len %u type %02d\n",
            dirent->name, dirent->inode, dirent->rec_len, dirent->name_len, dirent->file_type);

        if(strcmp(name, dirent->name) == 0)
        {
            ino = dirent->inode;
            break;
        }

        dirent = (ext2_dirent_t *) (((unsigned int)dirent) + dirent->rec_len);
    }
    
    ext2_free_block(blk);

    return ino;
}


void ext2_setup_fs()
{
    memset(&ext2_fs, 0, sizeof(ext2_fs));

    char *buf = kmalloc(EXT2_BLOCK_SIZE, 0);
    if(buf == 0)
        panic("out of memory");

    BLKRW(1, 1, buf);   // now blocksize == 1024, so blkid == 1

    memcpy(EXT2_SB, buf, sizeof(*(EXT2_SB)));

    if(EXT2_SB->s_magic != EXT2_SUPER_MAGIC)
    {
        printk("file system magic %04x\n", EXT2_SB->s_magic);
        panic("only support ext2 file system...");
    }

    printk("Ext2 File System Information:\n");
    printk(" inodes %u blocks %u free blocks %u free inodes %u\n",
        EXT2_SB->s_inodes_count, EXT2_SB->s_blocks_count, EXT2_SB->s_free_blocks_count, EXT2_SB->s_free_inodes_count);
    printk(" block size %u log block size %u first data block %u\n",
        EXT2_BLOCK_SIZE, EXT2_SB->s_log_block_size, EXT2_SB->s_first_data_block);
    printk(" blocks per group %u inodes per group %u\n", EXT2_SB->s_blocks_per_group, EXT2_SB->s_inodes_per_group);


    ext2_block_cache = kmem_cache_create("ext2_block_cache", EXT2_BLOCK_SIZE, EXT2_BLOCK_SIZE);
    if(0 == ext2_block_cache)
        panic("setup ext2 block cache failed. out of memory");

    ext2_inode_cache = kmem_cache_create("ext2_inode_cache", EXT2_INODE_SIZE, EXT2_INODE_SIZE);
    if(0 == ext2_inode_cache)
        panic("setup ext2 inode cache failed. out of memory");

    ext2_fs.ext2_gd = ext2_alloc_block();
    assert(ext2_fs.ext2_gd != 0);

    BLKRW(EXT2_SB->s_first_data_block+1, 1, (char *)ext2_fs.ext2_gd);

    unsigned int gps = EXT2_SB->s_blocks_count / EXT2_SB->s_blocks_per_group;
    gps += (EXT2_SB->s_blocks_count % EXT2_SB->s_blocks_per_group) ? 1 : 0;
    unsigned int i;
    for(i=0; i<gps; ++i)
    {
        printk("  [%2u] inode table %u free blocks %u free inode %u used dir %u\n",
            i, ext2_gd(i)->bg_inode_table, ext2_gd(i)->bg_free_blocks_count, ext2_gd(i)->bg_free_inodes_count, ext2_gd(i)->bg_used_dirs_count);
    }


    ext2_read_inode(2, &ext2_root_inode);
    printk("root inode size %u \n", ext2_root_inode.i_size);
    printk("root blocks %u \n", ext2_root_inode.i_blocks);


    ext2_read_inode(ext2_search_indir("boot", &ext2_root_inode), &boot_inode);
    ext2_read_inode(ext2_search_indir("Kernel", &boot_inode), &krnl_inode);
    printk("krnl inode size %u \n", krnl_inode.i_size);
    printk("krnl blocks %u \n", krnl_inode.i_blocks);
}




void setup_fs()
{
    ext2_setup_fs();
}

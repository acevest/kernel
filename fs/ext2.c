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

unsigned long ext2_block_size()
{
    return (EXT2_MIN_BLOCK_SIZE << (EXT2_SB)->s_log_block_size);
}

void *ext2_alloc_block()
{
    return (void *) kmem_cache_alloc(ext2_block_cache, 0);
}

void *ext2_free_block(void *blk)
{
    kmem_cache_free(ext2_block_cache, blk);
}

void *ext2_alloc_inode()
{
    return (void *) kmem_cache_alloc(ext2_inode_cache, 0);
}

#define ext2_gd(n) ((ext2_gd_t*)(EXT2_GD) + (n))
void ext2_read_inode(unsigned int ino, ext2_inode_t *inode)
{
    void *blk = ext2_alloc_block();
    assert(blk != 0);

    printk("read_inode %u\n", ino);

    unsigned int in;    // inode number
    unsigned int gn;    // group number
    unsigned int gi;    // inode index in group

    gn = (ino-1) / EXT2_INODES_PER_GROUP;
    gi = (ino-1) % EXT2_INODES_PER_GROUP;

    unsigned int blkid = gi / EXT2_INODES_PER_BLOCK; // inode blkid
    unsigned int inoff = gi % EXT2_INODES_PER_BLOCK; // inode offset

    blkid += ext2_gd(gn)->bg_inode_table;
    inoff *= EXT2_INODE_SIZE;

    printd("group %u %u blkid %u blkoff %u\n", gn, gi, blkid, inoff);

    BLKRW(blkid, 1, blk);

    memcpy(inode, blk+inoff, sizeof(ext2_inode_t));

    printk(" inode size %u \n", inode->i_size);

    ext2_free_block(blk);
}

void ext2_read_file(const ext2_inode_t *inode, char *buf)
{
    assert(inode != 0);
    assert(buf != 0);
    assert(inode->i_size > 0 && inode->i_size<=MAX_SUPT_FILE_SIZE);

    unsigned int blkcnt = inode->i_size / EXT2_BLOCK_SIZE;
    int i=0;
    for(i=0; i<blkcnt; ++i)
    {
        BLKRW(inode->i_block[i], 1, buf+i*EXT2_BLOCK_SIZE);
        printd("read block\n");
    }

    unsigned int left = inode->i_size % EXT2_BLOCK_SIZE;
    if(left)
    {
        printd("read left %u bytes\n", left);

        void *blk = ext2_alloc_block();

        memcpy(buf+i*EXT2_BLOCK_SIZE, blk, left);

        ext2_free_block(blk);
    }
    printd("read file done\n");
}

void ext2_read_data(const ext2_inode_t *inode, unsigned int offset, size_t size, char *buf)
{
    assert(inode != 0);
    assert(buf != 0);
    assert(inode->i_size > 0 && inode->i_size<=MAX_SUPT_FILE_SIZE);
    assert(offset+size <= inode->i_size);
    assert(offset % EXT2_BLOCK_SIZE == 0);  // for easy
    printk("offset %x size %x  %x\n", offset, size, offset+size);
    assert((offset+size) % EXT2_BLOCK_SIZE == 0);

    unsigned int blkid  = offset / EXT2_BLOCK_SIZE;
    unsigned int blkcnt = size   / EXT2_BLOCK_SIZE;
    printk("id %u cnt %u\n", blkid, blkcnt);
    BLKRW(inode->i_block[blkid], blkcnt, buf);
}


unsigned int ext2_search_indir(const char *name, const ext2_inode_t *inode)
{
    unsigned int ino = 0;

    void *blk = ext2_alloc_block();
    assert(blk != 0);

    BLKRW(inode->i_block[0], 1, blk); // only support the first direct blocks

    ext2_dirent_t *dirent = (ext2_dirent_t *) blk;
    char tmp[64];
    while(dirent->name_len != 0)
    {
        memcpy(tmp, dirent->name, dirent->name_len);
        tmp[dirent->name_len] = 0;
        printd("  dirent %s inode %u rec_len %u name_len %u type %02d\n",
            tmp, dirent->inode, dirent->rec_len, dirent->name_len, dirent->file_type);

        if(strcmp(name, tmp) == 0)
        {
            ino = dirent->inode;
            break;
        }

        dirent = (ext2_dirent_t *) (((unsigned int)dirent) + dirent->rec_len);
    }
    
    ext2_free_block(blk);

    return ino;
}

static int get_filename_from_path(const char *path, char *file)
{
    int i = 0;

    while(*path == '/' && *path != '\0')
        path++;

    while(*path != '/' && *path != '\0')
        file[i++] = *path++;

    file[i] = 0;

    return i;
}

unsigned int ext2_search_inpath(const char *path)
{
    assert(path != 0);
    assert(strlen(path) > 0);
    assert(path[0] == '/');

    ext2_inode_t *inode = kmalloc(sizeof(ext2_inode_t), 0);
    assert(inode != 0);
    memcpy(inode, &ext2_root_inode, sizeof(ext2_inode_t));

    unsigned int ino = EXT2_ROOT_INO;

    #define MAX_FILE_NAME 64
    char file[MAX_FILE_NAME];
    int len;

    while((len=get_filename_from_path(path, file)) != 0)
    {
        ino = ext2_search_indir(file, inode);
        assert(ino != 0);

        path += len;

        if(*path != 0)
        {
            path++;
            ext2_read_inode(ino, inode);
        }
        else
            assert(0);
    }

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
#if 1
    printk("root inode.i_size %u \n", ext2_root_inode.i_size);
    printk("root blocks %u \n", ext2_root_inode.i_blocks);

    ext2_read_inode(ext2_search_indir("boot", &ext2_root_inode), &boot_inode);
    ext2_read_inode(ext2_search_indir("Kernel", &boot_inode), &krnl_inode);
    printk("krnl inode.i_size %u \n", krnl_inode.i_size);
    printk("krnl blocks %u \n", krnl_inode.i_blocks);
#endif


    //printk("----- ino %u\n", ext2_search_inpath("/bin/test/test.txt"));
    //printk("----- ino %u\n", ext2_search_inpath("/bin/hello"));
    //printk("----- ino %u\n", ext2_search_inpath("/boot/Kernel"));
    //printk("----- ino %u\n", ext2_search_inpath("/boot/grub2/fonts/unicode.pf2"));
}





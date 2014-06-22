/*
 *--------------------------------------------------------------------------
 *   File Name: fs.h
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Fri Feb 12 22:29:59 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */

#ifndef _FS_H
#define _FS_H

#include <types.h>
#include <page.h>
#include <ext2.h>

typedef struct partition
{
    u8    State;
    u8    Head;
    u16    StartSC;
    u8    Type;
    u8    EndHead;
    u16    EndSC;
    u32    AbsoluteSectNo;
    u32    PartitionSize;
} Partition, *pPartition;


/* 分区表开始的位置 */
#define PARTS_POS    0x1BE

/* 设备的主设备号. 占用两个字节. */
#define DEV_MAJOR_UNUSED    0x0000
#define DEV_MAJOR_MEM       0x0001
#define DEV_MAJOR_TTY       0x0002
#define DEV_MAJOR_IDE0      0x0003
#define DEV_MAJOR_HD        DEV_MAJOR_IDE0
#define DEV_MAJOR_IDE1      0x0004
#define DEV_MAJOR_SCSI0     0x0005
#define DEV_MAJOR_SCSI2     0x0006

#define DEV_MAJOR_BITS        (16)
#define DEV_MINOR_MASK        ((1UL << DEV_MAJOR_BITS) - 1)

#define MAKE_DEV(major, minor)    ((major) << DEV_MAJOR_BITS | minor)

#define DEV_MAJOR(dev)        ((unsigned int)((dev) >> DEV_MAJOR_BITS))
#define DEV_MINOR(dev)        ((unsigned int)((dev) &  DEV_MINOR_MASK))



typedef struct
{
    int    count;
    int    ino_nr;
    pInode    inode;
} File, *pFile;

#define MAX_SUPT_FILE_SIZE    (EXT2_IND_BLOCK*EXT2_BLOCK_SIZE)
#define NR_FILES    (PAGE_SIZE/sizeof(File))
#define NR_INODES    (2*NR_FILES)
#define NR_OPENS    (2)    /* 一个进程同时打开文件的限制数 */
extern File file_table[NR_FILES];
extern Inode inode_table[NR_INODES];




static inline int get_inode_nr(const char *path)
{
    return ext2_get_file_inode_nr(path);
}

static inline int get_inode(unsigned int n, pInode inode)
{
    return ext2_read_inode(n, inode);
}

static inline int read_file(const pInode inode, void *buf, size_t count)
{
    return ext2_read_file(inode, buf, count);
}

/* 在多进程下这样肯定不行 
 * 管不了这么多了，先这样写吧
 */
static inline pInode find_empty_inode()
{
    int i;
    pInode p = inode_table;
    for(i=0; i<NR_FILES; i++, p++)
    {
        if(p->i_size == 0)
        {
            p->i_size = 1;
            return p;
        }
    }

    return NULL;
}

#endif //_FS_H

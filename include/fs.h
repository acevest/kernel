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

#include <page.h>
#include <types.h>

/* 分区表开始的位置 */
#define PARTS_POS 0x1BE

/* 设备的主设备号. 占用两个字节. */
#define DEV_MAJOR_UNUSED 0x0000
#define DEV_MAJOR_MEM 0x0001
#define DEV_MAJOR_TTY 0x0002
#define DEV_MAJOR_DISK 0x0003
#define DEV_MAJOR_IDE1 0x0004
#define DEV_MAJOR_SCSI0 0x0005
#define DEV_MAJOR_SCSI2 0x0006

#define DEV_MAJOR_BITS (16)
#define DEV_MINOR_MASK ((1UL << DEV_MAJOR_BITS) - 1)

#define MAKE_DEV(major, minor) ((major) << DEV_MAJOR_BITS | minor)
#define MAKE_DISK_DEV(drv_no, part_no) MAKE_DEV(DEV_MAJOR_DISK, (((drv_no)&0x03) << 8) | (((part_no)&0xFF) << 0))

#define DEV_MAJOR(dev) ((unsigned int)((dev) >> DEV_MAJOR_BITS))
#define DEV_MINOR(dev) ((unsigned int)((dev)&DEV_MINOR_MASK))

// #define MAX_SUPT_FILE_SIZE    (1)
#define NR_FILES (1)
#define NR_OPENS (1)

unsigned int namei(const char *path);

#define MAX_SUPT_FILE_SIZE (EXT2_IND_BLOCK * EXT2_BLOCK_SIZE)

typedef struct chrdev {
    int (*read)(char *buf, size_t count);
} chrdev_t;

enum { CHRDEV_CNSL, CHRDEV_SIZE };

extern chrdev_t *chrdev[];

typedef struct {
} file_t;

#if 0
#define NR_FILES (PAGE_SIZE / sizeof(File))
#define NR_INODES (2 * NR_FILES)
#define NR_OPENS (2) /* 一个进程同时打开文件的限制数 */
extern File file_table[NR_FILES];
extern Inode inode_table[NR_INODES];


typedef struct
{
    int    count;
    int    ino_nr;
    pInode    inode;
} File, *pFile;


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
#endif

typedef uint32_t dev_t;

#endif  //_FS_H

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
#include <vfs.h>

#include "stat.h"

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
#define MAKE_DISK_DEV(drvid, partid) MAKE_DEV(DEV_MAJOR_DISK, (((drvid) & 0x03) << 8) | (((partid) & 0xFF) << 0))

#define DEV_MAJOR(dev) ((unsigned int)((dev) >> DEV_MAJOR_BITS))
#define DEV_MINOR(dev) ((unsigned int)((dev) & DEV_MINOR_MASK))

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

typedef int (*fill_super_cb_t)(superblock_t *sb, void *data);
int read_super_for_nodev(fs_type_t *type, int flags, void *data, fill_super_cb_t fill_super, vfsmount_t *mnt);

int sget(fs_type_t *type,                      //
         int (*test)(superblock_t *, void *),  //
         int (*set)(superblock_t *, void *),   //
         void *data,                           //
         superblock_t **s                      // OUT
);

file_t *get_file(int fd);

#endif  //_FS_H

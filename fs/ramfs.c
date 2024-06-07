/*
 * ------------------------------------------------------------------------
 *   File Name: ramfs.c
 *      Author: Zhao Yanbai
 *              2024-04-13 23:43:49 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include "ramfs.h"

#include "fs.h"
#include "mm.h"
#include "string.h"
#include "system.h"
#include "vfs.h"

superblock_t *ramfs_read_super(superblock_t *sb, void *data);
fs_type_t ramfs_type = {
    .name = "ramfs",
    .read_super = ramfs_read_super,
    .next = 0,
};

typedef struct ramfs_superblock {
    //
} ramfs_superblock_t;

typedef struct ramfs_dentry {
    // d

} ramfs_dentry_t;

typedef struct ramfs_inode {
    // d
} ramfs_inode_t;

void ramfs_init() { vfs_register_filesystem(&ramfs_type); }

superblock_t *ramfs_read_super(superblock_t *sb, void *data) { return sb; }

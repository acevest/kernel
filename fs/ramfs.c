/*
 * ------------------------------------------------------------------------
 *   File Name: ramfs.c
 *      Author: Zhao Yanbai
 *              2024-04-13 23:43:49 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include "ramfs.h"

#include "errno.h"
#include "fs.h"
#include "mm.h"
#include "string.h"
#include "system.h"

typedef struct ramfs_superblock {
    //
} ramfs_superblock_t;

typedef struct ramfs_dentry {
    // d

} ramfs_dentry_t;

typedef struct ramfs_inode {
    // d
    inode_t vfs_inode;
} ramfs_inode_t;

static kmem_cache_t *g_ramfs_inode_cache = 0;

inode_t *ramfs_get_inode(superblock_t *sb, umode_t mode, dev_t dev);

// static inode_t *ramfs_alloc_inode(superblock_t *sb) {
//     ramfs_inode_t *inode = kmem_cache_alloc(g_ramfs_inode_cache, 0);
//     assert(inode != 0);

//     return &(inode->vfs_inode);
// }

static const file_operations_t ramfs_file_operations = {
    .read = 0,
};

static const inode_operations_t ramfs_file_inode_operations = {

};

static const file_operations_t ramfs_dir_operations = {

};

int ramfs_mkdir(inode_t *dir, dentry_t *dentry, int mode) {
    int ret = 0;

    inode_t *inode = NULL;

    inode = ramfs_get_inode(dir->i_sb, mode | S_IFDIR, 0);
    if (inode == NULL) {
        return -ENOSPC;
    }

    dentry_attach_inode(dentry, inode);

    dentry_get(dentry);

    return ret;
}

dentry_t *ramfs_lookup(inode_t *dir, dentry_t *dentry) {
    dentry_add(dentry, NULL);

    return NULL;
}

static const inode_operations_t ramfs_dir_inode_operations = {
    .mkdir = ramfs_mkdir,
    .lookup = ramfs_lookup,
};

inode_t *ramfs_get_inode(superblock_t *sb, umode_t mode, dev_t dev) {
    inode_t *inode = alloc_inode(sb);

    if (NULL == inode) {
        return inode;
    }

    inode->i_mode = mode;

    switch (mode & S_IFMT) {
    case S_IFREG:
        // panic("S_IFREG: not implement");
        inode->i_fops = &ramfs_file_operations;
        inode->i_ops = &ramfs_file_inode_operations;
        break;
    case S_IFDIR:
        // panic("S_IFDIR: not implement");
        inode->i_fops = &ramfs_dir_operations;
        inode->i_ops = &ramfs_dir_inode_operations;
        break;
    case S_IFLNK:
        panic("S_IFLNK: not implement");
        break;
    default:
        init_special_inode(inode, mode, dev);
        break;
    }

    return inode;
}

static sb_operations_t ramfs_ops = {
    // .alloc_inode = ramfs_alloc_inode,
};

int ramfs_fill_super_cb(superblock_t *sb, void *data) {
    int err = 0;

    // assert(sb->sb_ops != NULL);
    inode_t *fs_root_inode = ramfs_get_inode(sb, S_IFDIR, 0);
    assert(fs_root_inode != NULL);

    dentry_t *fs_root_dentry = 0;
    fs_root_dentry = dentry_alloc_root(fs_root_inode);
    assert(fs_root_dentry != NULL);

    sb->sb_root = fs_root_dentry;

    return err;
}

int ramfs_read_super(fs_type_t *type, int flags, const char *name, void *data, vfsmount_t *mnt) {
    int ret = 0;

    ret = read_super_for_nodev(type, flags, data, ramfs_fill_super_cb, mnt);

    return ret;
}

fs_type_t ramfs_type = {
    .name = "ramfs",
    .read_super = ramfs_read_super,
    .next = 0,
};

void ramfs_init() {
    vfs_register_filesystem(&ramfs_type);
    g_ramfs_inode_cache = kmem_cache_create("ramfs_inode_cache", sizeof(ramfs_inode_t), 4);
    assert(0 != g_ramfs_inode_cache);
}

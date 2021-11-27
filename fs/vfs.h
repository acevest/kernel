/*
 * ------------------------------------------------------------------------
 *   File Name: vfs.h
 *      Author: Zhao Yanbai
 *              2021-11-27 11:59:27 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

typedef struct vfs_dentry vfs_dentry_t;

typedef struct vfs_sb_operations vfs_sb_operations_t;
typedef struct vfs_inode_operations vfs_inode_operations_t;
typedef struct vfs_dentry_operations vfs_dentry_operations_t;

// super block
typedef struct vfs_sb {
    //
    vfs_dentry_t *root;
    void *private;
    vfs_sb_operations_t *sb_ops;
} vfs_sb_t;

typedef struct vfs_inode {
    //
    vfs_sb_t *sb;
    void *private;
    vfs_inode_operations_t *i_ops;
} vfs_inode_t;

struct vfs_dentry {
    //
    vfs_dentry_operations_t *d_ops;
};

struct vfs_sb_operations {
    //
};

struct vfs_inode_operations {
    //
};

struct vfs_dentry_operations {
    //
};

extern vfs_sb_t *root_sb;
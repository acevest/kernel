/*
 * ------------------------------------------------------------------------
 *   File Name: vfs.h
 *      Author: Zhao Yanbai
 *              2021-11-27 11:59:27 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <list.h>

typedef struct dentry dentry_t;

typedef struct sb_operations sb_operations_t;
typedef struct inode_operations inode_operations_t;
typedef struct dentry_operations dentry_operations_t;

// super block
typedef struct superblock {
    //
    dentry_t *sb_root;
    void *sb_private;
    sb_operations_t *sb_ops;
} superblock_t;

// dentry和inode为什么不合二为一？
// 每一个文件除了有一个索引节点inode(里面记录着文件在存储介质上的位置和分布信息)外还有一个目录项dentry
// 同时dentry还有个指针指向inode结构
// 这里inode 和 dentry 都是在从不同角度描述同个文件的各方面属性，不将他们合二为一的原因是
// dentry和inode描述的目标是不同的，因为一个文件可能有好几个文件名，通过不同的文件名访问同一个文件时权限也可能不同
// 所以dentry代表逻辑意义上的文件，记录的是逻辑意义上的属性
// 而inode结构代表的是物理意义上的文件
// 它们之间的关系是多对一的关系
typedef struct inode {
    superblock_t *i_sb;
    void *i_private;
    inode_operations_t *i_ops;

    // 缓存的pages
    list_head_t i_pages;
} inode_t;

struct dentry {
    char *d_name;

    //
    dentry_t *d_parent;

    // 同一目录里所有结点通过d_child链接在一起
    // 并连到它们父目录的d_subdirs队列中
    list_head_t d_child;
    list_head_t d_subdirs;

    //
    superblock_t *d_sb;

    // 每一个dentry指向一个inode
    // 但多个dentry可以指向同一个inode(不实现)
    inode_t *d_inode;

    //
    dentry_operations_t *d_ops;

    //
    void *d_private;
};

struct sb_operations {
    //
};

struct inode_operations {
    //
};

struct dentry_operations {
    //
};

// 每当将一个存储设备安装到现有文件系统中的某个节点时，内核就要为之建立一个vfsmount结构
// 这个结构中即包含着该设备的有关信息,也包含了安装点的信息
// 系统中的每个文件系统，包括根设备的根文件系统,都要经过安装
typedef struct vfsmount {
} vfsmount_t;

typedef struct fs_type {
    const char *name;
    superblock_t *(*read_super)(superblock_t *, void *);
    struct fs_type *next;
} fs_type_t;

extern superblock_t *root_sb;

void register_filesystem(fs_type_t *fs);

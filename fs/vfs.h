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
#include <types.h>

typedef struct dentry dentry_t;

typedef struct sb_operations sb_operations_t;
typedef struct file_operations file_operations_t;
typedef struct inode_operations inode_operations_t;
typedef struct dentry_operations dentry_operations_t;

// super block
typedef struct superblock {
    // 该超级起的根目录的 dentry
    dentry_t *sb_root;
    //
    void *sb_private;
    //
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

    // fops - file ops 的副本
    file_operations_t *i_fops;

    // ops - inode ops
    inode_operations_t *i_ops;

    // 缓存的pages
    list_head_t i_pages;

    dev_t i_dev;   // inode存储在该设备上
    dev_t i_rdev;  // inode所代表的设备号

    loff_t i_size;
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

    // 需要一个标记自己已经成为挂载点的标志？
    // uint32_t d_flags;
    // 也可以用一个指向vfsmount的指针，非0表示挂载？
    // 貌似应该搞个链表，因为一个点可以重复挂N次，N个文件系统
    list_head_t d_vfsmnt;  // 所有挂载到这个目录的挂载点

    //
    dentry_operations_t *d_ops;

    //
    void *d_private;
};

struct sb_operations {
    // read_inode
};

// struct file_operations {
//     // open
//     // close
//     // read
//     // write
//     // lseek
//     // ioctl

// };
struct inode_operations {
    //
};

struct dentry_operations {
    //
    // hash
    // compare
    // d_release 关闭文件
    // d_delete 删除文件
    //
};

// 每当将一个存储设备安装到现有文件系统中的某个节点时，内核就要为之建立一个vfsmount结构
// 这个结构中即包含着该设备的有关信息,也包含了安装点的信息
// 系统中的每个文件系统，包括根设备的根文件系统,都要经过安装
//
// 在安装文件系统时内核主要做如下的事情
// 创建一个vfsmount
// 为被安装的设备创建一个superblock，并由该设备对应的文件系统来设置这个superblock
// 为被安装的设备的根目录创建一个dentry
// 为被安装的设备的根目录创建一个inode，由sb->sb_ops->read_inode来实现
// 将superblock与被安装设备根目录的dentry关联
// 将vfsmount与被安装设备的根目录dentry关联
typedef struct vfsmount {
    dentry_t *mount_point;  // 挂载点 dentry
    dentry_t *root;         // 设备根目录 dentry
    superblock_t *sb;       // 被安装的设备的superblock
    struct vfsmount *parent;
    // sb->sb_ops->read_inode得到被安装设备根目录的inode

    list_head_t list;  // vfsmount 链表

    // 先简单实现：不支持一个设备挂载多次，或一个目录被挂载多次
} vfsmount_t;

typedef struct fs_type {
    const char *name;
    superblock_t *(*read_super)(superblock_t *, void *);
    struct fs_type *next;
    list_head_t sbs;  // 同属于这个文件系统的所有超级块链表
} fs_type_t;

extern superblock_t *root_sb;

void vfs_register_filesystem(fs_type_t *fs);

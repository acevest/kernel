/*
 * ------------------------------------------------------------------------
 *   File Name: vfs.h
 *      Author: Zhao Yanbai
 *              2021-11-27 11:59:27 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <atomic.h>
#include <list.h>
#include <page.h>
#include <semaphore.h>
#include <types.h>

typedef struct qstr {
    const char *name;
    unsigned int len;
    uint64_t hash;
} qstr_t;

typedef struct dentry dentry_t;
typedef struct inode inode_t;
typedef struct sb_operations sb_operations_t;
typedef struct file_operations file_operations_t;
typedef struct inode_operations inode_operations_t;
typedef struct dentry_operations dentry_operations_t;
typedef struct vfsmount vfsmount_t;
typedef struct path path_t;

typedef struct address_space_operations address_space_operations_t;
typedef struct address_space address_space_t;

struct path {
    dentry_t *dentry;
    vfsmount_t *mnt;
};

#define PATH_LOOKUP_PARENT /*    */ 0x00000001
#define PATH_LOOKUP_DIRECTORY /* */ 0x00000002
#define PATH_LOOKUP_MUST_HAVE_INODE 0x00000004

enum {
    LAST_FORGET_INIT = 0,
    LAST_NORMAL = 1,
    LAST_ROOT,
    LAST_DOT,
    LAST_DOTDOT,
};

typedef struct namei {
    path_t path;
    qstr_t last;
    uint32_t last_type;
    uint32_t flags;
} namei_t;

typedef struct file file_t;

typedef struct file_operations {
    int (*open)(inode_t *, file_t *);
    int (*release)(inode_t *, file_t *);
    ssize_t (*read)(file_t *, char *, size_t, loff_t *);
    ssize_t (*write)(file_t *, const char *, size_t, loff_t *);
} file_operations_t;

struct file {
    // 多个打开的文件可能是同一个文件
    dentry_t *f_dentry;
    const file_operations_t *f_ops;

    loff_t f_pos;
    uint32_t f_flags;
};

// super block
typedef struct superblock {
    // 该超级起的根目录的 dentry
    dentry_t *sb_root;

    int sb_flags;

    //
    void *sb_private;
    //
    sb_operations_t *sb_ops;

    list_head_t sb_list;
    list_head_t sb_instance;

    dev_t sb_dev;
} superblock_t;

struct address_space_operations {
    int (*write_page)(page_t *);
    int (*read_page)(file_t *file, page_t *);
    int (*write_begin)(file_t *file, page_t *, loff_t pos, int len);
    int (*write_end)(file_t *file, page_t *, loff_t pos, int len);
};

struct address_space {
    list_head_t pages;
    uint32_t total_pages;
    inode_t *a_inode;
    const address_space_operations_t *a_ops;
};

// dentry和inode为什么不合二为一？
// 每一个文件除了有一个索引节点inode(里面记录着文件在存储介质上的位置和分布信息)外还有一个目录项dentry
// 同时dentry还有个指针指向inode结构
// 这里inode 和 dentry 都是在从不同角度描述同个文件的各方面属性，不将他们合二为一的原因是
// dentry和inode描述的目标是不同的，因为一个文件可能有好几个文件名，通过不同的文件名访问同一个文件时权限也可能不同
// 所以dentry代表逻辑意义上的文件，记录的是逻辑意义上的属性
// 而inode结构代表的是物理意义上的文件
// 它们之间的关系是多对一的关系
struct inode {
    superblock_t *i_sb;

    void *i_private;

    semaphore_t i_sem;

    // fops - file ops 的副本
    const file_operations_t *i_fops;

    // ops - inode ops
    const inode_operations_t *i_ops;

    // 缓存的pages
    list_head_t i_pages;

    dev_t i_dev;   // inode存储在该设备上
    dev_t i_rdev;  // inode所代表的设备号

    loff_t i_size;

    umode_t i_mode;  // FILE DIR CHR BLK FIFO SOCK

    address_space_t *i_mapping;
    address_space_t i_as;
};

// d_flags
#define DENTRY_FLAGS_MOUNTED 0x01

// d_inline_name
#define DENTRY_INLINE_NAME_LEN 16

struct dentry {
    // char *d_name;
    uint32_t d_flags;
    qstr_t d_name;
    char d_inline_name[DENTRY_INLINE_NAME_LEN];

    atomic_t d_count;

    //
    dentry_t *d_parent;

    // 同一目录里所有结点通过d_child链接在一起
    // 并连到它们父目录的d_subdirs队列中
    list_head_t d_child;
    list_head_t d_subdirs;

    list_head_t d_hash;

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
    // alloc inode
    inode_t *(*alloc_inode)(superblock_t *sb);
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
    // 用于在inode下找一个dentry->d_small_name的目录项
    dentry_t *(*lookup)(inode_t *, dentry_t *);

    // 在inode下创建一个dentry->d_small_name的文件
    int (*create)(inode_t *, dentry_t *, umode_t, namei_t *);

    // 创建文件夹
    int (*mkdir)(inode_t *, dentry_t *, umode_t);

    // link
    // unlink
    // symlink
    // link 是普通连接 symlink是符号连接
    // 连接是指一个节点(文件或目录项)直接指向另一个节点，成为为该节点的一个代表
    // link必需处于同一个设备上，必需连接到一个真实的文件上
    // symlink可以处于不同的设备上，可以悬空，也就是没有连接到真实的文件上
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
#define MAX_VFSMNT_NAME_LEN 32
struct vfsmount {
    dentry_t *mnt_point;   // 挂载点 dentry
    dentry_t *mnt_root;    // 设备根目录 dentry
    superblock_t *mnt_sb;  // 被安装的设备的superblock
    struct vfsmount *mnt_parent;  // 如果多个设备挂载到同一个目录，则每个vfsmount的parent都指向同一个vfsmount
    // sb->sb_ops->read_inode得到被安装设备根目录的inode

    list_head_t mnt_list;  // vfsmount 链表

    vfsmount_t *hash_next;

    char mnt_devname[MAX_VFSMNT_NAME_LEN];

    // list_head_t mnt_clash;

    // 先简单实现：不支持一个设备挂载多次，或一个目录被挂载多次
};

#define FS_TYPE_NODEV 1
#define FS_TYPE_BLKDEV 2

typedef struct fs_type fs_type_t;
struct fs_type {
    const char *name;
    // superblock_t *(*read_super)(superblock_t *, void *);
    int (*read_super)(fs_type_t *type, int flags, const char *name, void *data, vfsmount_t *mnt);
    int flags;  // FS_REQUIRES_DEV or NODEV
    struct fs_type *next;
    // 同属于这个文件系统的所有超级块链表
    // 因为同名文件系统可能有多个实例，所有的该文件系统的实例的superblock,都通过sbs这个链到一起
    list_head_t sbs;
};

extern superblock_t *root_sb;

int vfs_register_filesystem(fs_type_t *fs);
fs_type_t *vfs_find_filesystem(const char *name);
/////

int vfs_create(inode_t *dir, dentry_t *dentry, int mode, namei_t *ni);
int vfs_mkdir(inode_t *dir, dentry_t *dentry, int mode);
////

inode_t *alloc_inode(superblock_t *sb);
void init_special_inode(inode_t *inode, umode_t mode, dev_t rdev);
////

int dentry_cached_lookup(dentry_t *parent,  //
                         qstr_t *s,         //
                         dentry_t **dentry  // OUT
);

int dentry_real_lookup(dentry_t *parent,  //
                       qstr_t *s,         //
                       dentry_t **dentry  // OUT
);

dentry_t *dentry_alloc_root(inode_t *root_inode);
dentry_t *dentry_alloc(dentry_t *parent, const qstr_t *s);
void dentry_add(dentry_t *dentry, inode_t *inode);
void dentry_attach_inode(dentry_t *dentry, inode_t *inode);

vfsmount_t *vfsmnt_get(vfsmount_t *m);
void vfsmnt_put(vfsmount_t *m);

dentry_t *dentry_get(dentry_t *dentry);
void dentry_get_locked(dentry_t *dentry);

void dentry_put(dentry_t *dentry);

//
bool path_init(const char *path, unsigned int flags, namei_t *ni);
int path_walk(const char *path, namei_t *ni);
int path_lookup_create(namei_t *ni,       //
                       dentry_t **dentry  // OUT
);

int path_open_namei(const char *path, int flags, int mode, namei_t *ni);

//
ssize_t vfs_generic_file_read(file_t *file, char *buf, size_t size, loff_t *p_pos);
ssize_t vfs_generic_file_write(file_t *file, const char *buf, size_t size, loff_t *p_pos);

/*
 * ------------------------------------------------------------------------
 *   File Name: inode.c
 *      Author: Zhao Yanbai
 *              2024-05-15 20:32:55 Wednesday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include "assert.h"
#include "fs.h"
#include "mm.h"
#include "printk.h"
#include "system.h"

static kmem_cache_t *g_inode_kmem_cache = NULL;

inode_t *alloc_inode(superblock_t *sb) {
    inode_t *inode = 0;

    assert(NULL != sb->sb_ops);
    // assert(NULL != sb->sb_ops->alloc_inode);

    if (NULL != sb->sb_ops->alloc_inode) {
        inode = sb->sb_ops->alloc_inode(sb);
    } else {
        inode = kmem_cache_alloc(g_inode_kmem_cache, 0);
    }

    if (0 == inode) {
        printk("alloc inode fail");
    }

    static file_operations_t empty_fops;
    static inode_operations_t empty_iops;
    inode->i_sb = sb;
    semaphore_init(&inode->i_sem, 1);
    inode->i_fops = &empty_fops;
    inode->i_ops = &empty_iops;
    inode->i_size = 0;
    inode->i_mapping = &inode->i_as;
    inode->i_mapping->a_inode = inode;
    inode->i_mapping->pages;
    INIT_LIST_HEAD(&inode->i_mapping->pages);
    inode->i_mapping->a_ops = 0;
    return inode;
}

void init_special_inode(inode_t *inode, umode_t mode, dev_t rdev) {
    inode->i_mode = mode;
    if (S_ISCHR(mode)) {
        panic("todo");
    } else if (S_ISBLK(mode)) {
        panic("todo");
    } else {
        panic("todo");
    }
}

void inode_cache_init() {
    g_inode_kmem_cache = kmem_cache_create("inode_kmem_cache", sizeof(inode_t), 4);
    assert(g_inode_kmem_cache != NULL);

    //
}

/*
 *--------------------------------------------------------------------------
 *   File Name: fs.c
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Fri Feb 12 20:48:50 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */
#include <fs.h>
#include <io.h>
#include <mm.h>
#include <printk.h>
#include <system.h>

// void ramfs_setup();
// void ext2_setup();

// void setup_fs() {
//     ramfs_setup();

//     // ext2_setup();
// }

//--------------------------------------------------------------------------

extern chrdev_t cnsl_chrdev;

chrdev_t *chrdev[CHRDEV_SIZE] = {&cnsl_chrdev};

// void ext2_setup_fs();
unsigned int ext2_search_inpath(const char *path);

unsigned int namei(const char *path) { return ext2_search_inpath(path); }

vfsmount_t rootfs_vfsmount;

dentry_t rootfs_root_dentry;

void setup_fs() {
    // ext2_setup_fs();

    void inode_cache_init();
    inode_cache_init();

    void dentry_cache_init();
    dentry_cache_init();

    void ramfs_init();
    ramfs_init();

    void init_mount();
    init_mount();
}

/*
 * ------------------------------------------------------------------------
 *   File Name: fssysc.c
 *      Author: Zhao Yanbai
 *              2024-09-01 21:41:00 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include "errno.h"
#include "fs.h"
#include "system.h"

//////
int vfs_mkdir(inode_t *dir, dentry_t *dentry, int mode) {
    int ret = 0;

    // TODO REMOVE
    assert(dir->i_ops->mkdir != NULL);

    if (dir->i_ops->mkdir == NULL) {
        return -EPERM;
    }

    ret = dir->i_ops->mkdir(dir, dentry, mode);

    if (0 != ret) {
        printk("%s ret %d\n", __func__, ret);
    }

    return ret;
}

__attribute__((regparm(0))) long sysc_mkdir(const char *path, int mode) {
    int ret = 0;

    // TODO 检查参数

    namei_t ni;
    if (path_init(path, PATH_LOOKUP_PARENT, &ni)) {
        ret = path_walk(path, &ni);
        if (0 != ret) {
            return ret;
        }
    }

    dentry_t *dentry;
    dentry = path_lookup_create(&ni);
    ret = PTR_ERR(dentry);
    if (!IS_ERR(dentry)) {
        ret = vfs_mkdir(ni.path.dentry->d_inode, dentry, mode);
        dentry_put(dentry);
    }

    up(&ni.path.dentry->d_inode->i_sem);

    return ret;
}

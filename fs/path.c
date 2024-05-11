/*
 * ------------------------------------------------------------------------
 *   File Name: path.c
 *      Author: Zhao Yanbai
 *              2024-04-20 19:24:42 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <errno.h>
#include <fs.h>
#include <sched.h>
#include <types.h>
#include <vfs.h>

bool path_init(const char *path, unsigned int flags, namei_t *ni) {
    if (path == NULL) {
        return false;
    }

    if (*path == '/') {
        ni->mnt = current->mnt_root;
        ni->dentry = current->dentry_root;
    }

    ni->mnt = current->mnt_pwd;
    ni->dentry = current->dentry_pwd;

    return true;
}

void follow_dotdot(namei_t *ni) {
    while (1) {
        dentry_t *dentry = NULL;
        vfsmount_t *parent = NULL;

        // 如果当前目录已经是根目录
        if (ni->dentry == current->dentry_root) {
            assert(ni->mnt == current->mnt_root);
            // do nothing
            return;
        }

        // 如果当前目录不是挂载的根目录
        if (ni->dentry != ni->mnt->root) {
            dentry = ni->dentry->d_parent;
            ni->dentry = dentry;
            return;
        }

        // 当前目录是挂载的根目录，这就需要找到挂载点的上一层目录
        // 所以先找到挂载点的父挂载点vfsmount
        parent = ni->mnt->parent;

        // 如果当前挂载点已经是最开始的挂载点vfsmount了
        if (ni->mnt == parent) {
            return;
        }

        // 如果当前挂载点有父挂载点
        // 就记录当前挂载点的dentry和它所在的挂载vfsmount
        dentry = ni->mnt->mount_point;
        ni->dentry = dentry;
        ni->mnt = parent;

        // 此时有两种情况
        // 1. dentry 是根目录，就需要继续往上找
        // 2. dentry 不是根目录，也需要再往上找一层dentry
    }
}

int path_walk(const char *path, namei_t *ni) {
    if (path == NULL) {
        return -EINVAL;
    }

    // 先跳过最开始的 '/'
    while (*path == '/') {
        path++;
    }

    // 所有连续'/'之后立即就是'\0'
    if (*path == 0) {
        return 0;
    }

    // 拿到当前目录的 inode
    inode_t *inode = ni->dentry->d_inode;

    while (1) {
        qstr_t this;
        this.name = path;

        do {
            path++;
        } while (*path && (*path != '/'));

        this.len = path - this.name;

        // 看是不是路径的最后一个文件名
        if (*path == 0) {
            //
        }

        // 跳过所有连续的 '/'
        while (*path == '/') {
            path++;
        }

        // 看是不是路径其实是以'/'结尾的
        if (*path == 0) {
            //
        }

        // 开始解析name(到这里这个name后跟着的所有的'/'都已经跳过了)
        if (this.name[0] == '.') {
            if (this.len == 1) {
                // 目录无变化，继续下一轮解析
                continue;
            } else if (this.len == 2 && this.name[1] == '.') {
                follow_dotdot(ni);
                inode = ni->dentry->d_inode;
                continue;
            }
        }

        //
    }

    return 0;
}

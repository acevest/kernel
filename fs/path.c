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
        if (ni->dentry != ni->mnt->mnt_root) {
            dentry = ni->dentry->d_parent;
            ni->dentry = dentry;
            return;
        }

        // 当前目录是挂载的根目录，这就需要找到挂载点的上一层目录
        // 所以先找到挂载点的父挂载点vfsmount
        parent = ni->mnt->mnt_parent;

        // 如果当前挂载点已经是最开始的挂载点vfsmount了
        if (ni->mnt == parent) {
            return;
        }

        // 如果当前挂载点有父挂载点
        // 就记录当前挂载点的dentry和它所在的挂载vfsmount
        dentry = ni->mnt->mnt_point;
        ni->dentry = dentry;
        ni->mnt = parent;

        // 此时有两种情况
        // 1. dentry 是根目录，就需要继续往上找
        // 2. dentry 不是根目录，也需要再往上找一层dentry

        // 关于这个循环可能处理的情况举例如下：

        // case1
        // 以 /root/aa/.. 为例 代表的是 /root
        // 如果 挂载一个目录到 /root/aa/
        // 这时 dentry 指向的是 aa
        // 还需要再往上走一层

        // case2
        // 假设 /mnt/a 挂载到 /mnt/b 后， /mnt/b 再挂载到 /mnt/c
        // 分别产生vfsmnt_a vfsmnt_b
        // 对于路径/mnt/c/..当读到c时，对于它的dentry，会最终读到a的dentry
        // 对于..
        // 第一次循环 dentry为b的dentry，mnt为 vfsmnt_b
        // 第二次循环 dentry为c的dentry, mnt为 vfsmnt_a
        // 第三次循环，dentry为mnt的dentry，mnt为mnt_root

        // case3
        // 假设 /mnt/a 挂载到 /mnt/c 后， /mnt/b 再挂载到 /mnt/c
        // 分别产生 vfsmnt_a vfsmnt_b
        // 这两个vfsmnt都会挂载到dentry->d_vfsmnt链表上
    }
}

uint64_t compute_qstr_hash(qstr_t *q) {
    q->hash = 0;
    for (int i = 0; i < q->len; i++) {
        uint64_t x = (uint64_t)(q->name[i]);
        q->hash = (x << 4) | (x >> (8 * sizeof(q->hash) - 4));
    }

    q->hash += q->hash >> (4 * sizeof(q->hash));

    return q->hash;
}

int follow_down(dentry_t **dentry, vfsmount_t **vfsmnt) {
    assert(*dentry != NULL);
    assert(*vfsmnt != NULL);

    list_head_t *pos;
    list_for_each(pos, &((*dentry)->d_vfsmnt)) {
        vfsmount_t *tmp_mnt = list_entry(pos, vfsmount_t, mnt_clash);
        if (*vfsmnt == tmp_mnt->mnt_parent) {
            *vfsmnt = vfsmnt_get(tmp_mnt);
            vfsmnt_put(tmp_mnt->mnt_parent);

            dentry_put(*dentry);
            *dentry = dentry_get(tmp_mnt->mnt_root);
        }
    }
}

int path_walk(const char *path, namei_t *ni) {
    dentry_t *dentry;
    int ret = 0;
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

    while (true) {
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
        compute_qstr_hash(&this);

        // 根据该名字，先上dentry cache里找
        dentry = dentry_cached_lookup(ni->dentry, &this);

        // 如果找不到就上实际存储设备中去找
        if (NULL == dentry) {
            ret = dentry_real_lookup(ni->dentry, &this, &dentry);
            if (0 != ret) {
                break;
            }
        }

        // 找到了，先看看它是不是一个挂载点
        while (!list_empty(&dentry->d_vfsmnt)) {
            // 如果是一个挂载点，则更进一步

            // 这种解决的是先将/a 挂载到 /b
            // 再将/b挂载到/c
            // 读/c时，直接读到/a的dentry
        }
    }

    return ret;
}

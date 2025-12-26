/*
 * ------------------------------------------------------------------------
 *   File Name: path.c
 *      Author: Zhao Yanbai
 *              2024-04-20 19:24:42 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <errno.h>
#include <fcntl.h>
#include <fs.h>
#include <sched.h>
#include <system.h>
#include <types.h>
#include <vfs.h>

bool path_init(const char *path, unsigned int flags, namei_t *ni) {
    ni->flags = flags;
    ni->last_type = LAST_ROOT;

    if (path == NULL) {
        return false;
    }

    if (*path == '/') {
        // ni->path.mnt = current->mnt_root;
        // ni->path.dentry = current->dentry_root;
        ni->path = current->root;
        return true;
    }

    // ni->path.mnt = current->mnt_pwd;
    // ni->path.dentry = current->dentry_pwd;
    ni->path = current->pwd;

    return true;
}

void follow_dotdot(namei_t *ni) {
#if 1
    panic("not supported");
#else
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
#endif
}

uint64_t compute_qstr_hash(qstr_t *q) {
    q->hash = 0;
    for (int i = 0; i < q->len; i++) {
        uint64_t c = (uint64_t)(q->name[i]);
        uint64_t x = q->hash;
        q->hash = (x << 4) | (x >> (8 * sizeof(q->hash) - 4));
        q->hash ^= c;
    }

    q->hash += q->hash >> (4 * sizeof(q->hash));

    return q->hash;
}

int follow_down(dentry_t **dentry, vfsmount_t **vfsmnt) {
    // assert(*dentry != NULL);
    // assert(*vfsmnt != NULL);

    // list_head_t *pos;
    // list_for_each(pos, &((*dentry)->d_vfsmnt)) {
    //     vfsmount_t *tmp_mnt = list_entry(pos, vfsmount_t, mnt_clash);
    //     if (*vfsmnt == tmp_mnt->mnt_parent) {
    //         *vfsmnt = vfsmnt_get(tmp_mnt);
    //         vfsmnt_put(tmp_mnt->mnt_parent);

    //         dentry_put(*dentry);
    //         *dentry = dentry_get(tmp_mnt->mnt_root);
    //     }
    // }
}

int path_walk(const char *path, namei_t *ni) {
    dentry_t *dentry = NULL;
    int ret = 0;
    if (path == NULL) {
        return -EINVAL;
    }

    // 先跳过最开始的 '/'
    while (*path == '/') {
        path++;
    }

    // 所有连续'/'之后立即就是'\0'就当作 '/'处理
    if (*path == 0) {
        return 0;
    }

    // 拿到当前目录的 inode
    inode_t *inode = ni->path.dentry->d_inode;

    uint32_t path_lookup_flags = ni->flags;

    while (true) {
        qstr_t this;
        this.name = path;
        do {
            path++;
        } while (*path != 0 && (*path != '/'));

        this.len = path - this.name;

        // 看是不是路径的最后一个文件名
        if (*path == 0) {
            goto last_file_name;
        }

        // 继续解析路径，此时this.name肯定带上了 '/'，现在就是要确定它是不是最后一个文件名
        // 首先，跳过所有连续的 '/'
        while (*path == '/') {
            path++;
        }

        // 若路径是以'/'结尾的，则肯定是最后一个文件名
        // 但这种情况下，最后这个文件名必需是文件夹名
        if (*path == 0) {
            goto last_file_name_with_slash;
        }

        // 若除了'/'又遇到了其它字符，则代表当前不是最后一个文件名

        // 开始解析name(到这里这个name后跟着的所有的'/'都已经跳过了)
        if (this.name[0] == '.') {
            if (this.len == 1) {
                // 目录无变化，继续下一轮解析
                continue;
            } else if (this.len == 2 && this.name[1] == '.') {
                // 跳到父目录
                follow_dotdot(ni);
                inode = ni->path.dentry->d_inode;
                continue;
            }
        }

        // 计算当前文件名的hash
        compute_qstr_hash(&this);

        // 根据该名字，先上dentry cache里找
        ret = dentry_cached_lookup(ni->path.dentry, &this, &dentry);
        assert(0 == ret);

        // 如果找不到就上实际存储设备中去找
        if (NULL == dentry) {
            ret = dentry_real_lookup(ni->path.dentry, &this, &dentry);
            if (0 != ret) {
                break;
            }
        }

        // 找到了，先看其是不是一个挂载点
        // TODO
        if (dentry->d_flags & DENTRY_FLAGS_MOUNTED) {
            panic("not supported");
        }

        // 不是挂载点 或已经处理完了挂载点
        ret = -ENOENT;
        inode = dentry->d_inode;
        if (inode == NULL) {
            goto out_dput_entry;
        }

        ret = -ENOTDIR;
        if (!S_ISDIR(inode->i_mode)) {
            goto out_dput_entry;
        }
        if (inode->i_ops == NULL) {
            goto out_dput_entry;
        }

        // TODO 判断 symlink
        // ...

        dentry_put(ni->path.dentry);
        ni->path.dentry = dentry;
        assert(inode->i_ops->lookup != NULL);

        // 进入下轮解析
        continue;

        // ---------------- 以下处理其它逻辑 ----------------

    last_file_name_with_slash:
        path_lookup_flags |= PATH_LOOKUP_DIRECTORY;
    last_file_name:

        // 计算当前文件名的hash
        compute_qstr_hash(&this);
        printk("HASH %s %lu\n", this.name, this.hash);

        if (path_lookup_flags & PATH_LOOKUP_PARENT) {
            ni->last = this;
            ni->last_type = LAST_NORMAL;
            if (this.len == 1 && this.name[0] == '.') {
                ni->last_type = LAST_DOT;
            }
            if (this.len == 2 && this.name[0] == '.' && this.name[1] == '.') {
                ni->last_type = LAST_DOTDOT;
            }
            goto ok;
        }

        // 最后一个文件名可能是 '.' 或 '..'
        if (this.len == 1 && this.name[0] == '.') {
            goto ok;
        }

        if (this.len == 2 && this.name[0] == '.' && this.name[1] == '.') {
            follow_dotdot(ni);
            inode = ni->path.dentry->d_inode;
            goto ok;
        }

        // // 计算当前文件名的hash
        // compute_qstr_hash(&this);
        // printk("HASH %s %lu\n", this.name, this.hash);

        // 根据该名字，先上dentry cache里找
        ret = dentry_cached_lookup(ni->path.dentry, &this, &dentry);
        assert(0 == ret);

        // 如果找不到就上实际存储设备中去找
        if (NULL == dentry) {
            ret = dentry_real_lookup(ni->path.dentry, &this, &dentry);
            if (0 != ret) {
                break;
            }
        }

        // 找到了，先看其是不是一个挂载点
        // TODO
        if (dentry->d_flags & DENTRY_FLAGS_MOUNTED) {
            panic("not supported");
        }

        inode = dentry->d_inode;
        // TODO 判断 symlink
        // ...

        dentry_put(dentry);
        ni->path.dentry = dentry;

        ret = -ENOENT;
        if (inode == NULL) {
            if (path_lookup_flags & (PATH_LOOKUP_DIRECTORY | PATH_LOOKUP_MUST_HAVE_INODE)) {
                goto end;
            }

            goto ok;
        }

        if (path_lookup_flags & PATH_LOOKUP_MUST_HAVE_INODE) {
            ret = -ENOTDIR;
            if (inode->i_ops == NULL || inode->i_ops->lookup == NULL) {
                goto end;
            }
        }
    ok:
        return 0;
    }

out_dput_entry:
    dentry_put(dentry);
end:
    return ret;
}

int path_lookup_hash(dentry_t *base, qstr_t *name, dentry_t **dentry) {
    int ret = 0;
    inode_t *inode = base->d_inode;

    ret = dentry_cached_lookup(base, name, dentry);
    assert(0 == ret);
    if (*dentry != NULL) {
        return 0;
    }

    dentry_t *dentry_new = dentry_alloc(base, name);
    if (dentry_new == NULL) {
        return ENOMEM;
    }

    *dentry = inode->i_ops->lookup(inode, dentry_new);

    if (*dentry == NULL) {  // 返回 lookup 没有再分配一个dentry
        *dentry = dentry_new;
    } else {  // 否则就释放dentry_new使用lookup返回的dentry
        dentry_put(dentry_new);
    }

    return ret;
}

int path_lookup_create(namei_t *ni, dentry_t **dentry) {
    int err = 0;

    // 在调用完path_lookup_create后调用 up 操作
    down(&ni->path.dentry->d_inode->i_sem);

    //
    if (ni->last_type != LAST_NORMAL) {
        return EEXIST;
    }

    err = path_lookup_hash(ni->path.dentry, &ni->last, dentry);
    if (err != 0) {
        return err;
    }

    return err;
}

int path_open_namei(const char *path, int flags, int mode, namei_t *ni) {
    int ret = 0;
    dentry_t *dentry = NULL;
    dentry_t *dir = NULL;
    inode_t *inode = NULL;

    if ((flags & O_CREAT) == 0) {
        path_init(path, flags, ni);
        ret = path_walk(path, ni);
        if (0 != ret) {
            return ret;
        }
        dentry = ni->path.dentry;
        goto ok;
    }

    path_init(path, PATH_LOOKUP_PARENT, ni);
    ret = path_walk(path, ni);
    if (0 != ret) {
        return ret;
    }

    if (ni->last_type != LAST_NORMAL) {
        ret = -EISDIR;
        goto end;
    }

    dir = ni->path.dentry;
    assert(NULL != dir);
    down(&dir->d_inode->i_sem);

    ret = path_lookup_hash(dir, &ni->last, &dentry);
    if (0 != ret) {
        up(&dir->d_inode->i_sem);
        goto end;
    }

    assert(dentry != NULL);
    if (NULL == dentry->d_inode) {
        ret = vfs_create(dir->d_inode, dentry, mode, ni);
        up(&dir->d_inode->i_sem);
        dentry_put(ni->path.dentry);
        ni->path.dentry = dentry;
        if (0 != ret) {
            goto end;
        }
        goto ok;
    }

    // 上述是文件不存在的逻辑
    // 此处是文件存在的情况下的处理逻辑
    up(&dir->d_inode->i_sem);

    if ((flags & O_EXCL) == 0) {
        panic("unsupport O_EXCL");
    }

ok:
    inode = dentry->d_inode;
    if (NULL == inode) {
        ret = ENOENT;
        goto end;
    }

    if (S_ISDIR(inode->i_mode)) {
        ret = EISDIR;
        goto end;
    }

    if ((flags & O_TRUNC) != 0) {
        panic("unsupport O_TRUNC");
    }

end:

    return ret;
}

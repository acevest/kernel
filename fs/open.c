/*
 *--------------------------------------------------------------------------
 *   File Name: open.c
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sat Feb 20 18:53:47 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fs.h>
#include <sched.h>
#include <syscall.h>
#include <types.h>

int get_unused_fd() {
    int fd;
    task_files_t* files = &(current->files);

    for (int i = 0; i < NR_TASK_OPEN_CNT; i++) {
        if (files->fds[i] == 0) {
            return i;
        }
    }

    printk("too many open files for %s\n", current->name);
    return -EMFILE;
}

int filp_open(const char* path, int flags, int mode, file_t** fp) {
    int ret = 0;

    assert(path != NULL);

    *fp = get_empty_filp();
    if (*fp == NULL) {
        return -ENFILE;
    }

    namei_t ni;
    path_open_namei(path, flags, mode, &ni);

    (*fp)->f_dentry = ni.path.dentry;
    (*fp)->f_flags = flags;
    (*fp)->f_ops = (*fp)->f_dentry->d_inode->i_fops;
    (*fp)->f_pos = 0;

    assert((*fp)->f_ops != NULL);

    // TODO: 添加open支持
    assert((*fp)->f_ops->open == NULL);

    return ret;
}

int sysc_open(const char* path, int flags, int mode) {
    int fd = 0;

    fd = get_unused_fd();

    if (fd < 0) {
        return fd;
    }

    file_t* fp;

    int ret = filp_open(path, flags, mode, &fp);
    if (ret != 0) {
        return ret;
    }

    current->files.fds[fd] = fp;

    return fd;
}

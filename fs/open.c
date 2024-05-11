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
    task_files_t *files = &(current->files);

    for (int i = 0; i < NR_TASK_OPEN_CNT; i++) {
        if (files->fds[i] == 0) {
            return i;
        }
    }

    printk("too many open files for %s\n", current->name);
    return -EMFILE;
}

file_t *filp_open(const char *path, int flags, mode_t mode) {
    int ret = 0;

    // ret = open_path(path, flags, mode, nd);

    return NULL;
}

int sysc_open(const char *path, int flags, mode_t mode) {
    int fd = 0;

    fd = get_unused_fd();

    if (fd < 0) {
        return fd;
    }

    file_t *fp = filp_open(path, flags, mode);

    current->files.fds[fd] = fp;

    return 0;
}

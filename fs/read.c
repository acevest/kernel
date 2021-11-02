/*
 *--------------------------------------------------------------------------
 *   File Name: read.c
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Tue Feb 23 18:53:47 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */
#include <assert.h>
#include <errno.h>
#include <fs.h>
#include <sched.h>
#include <types.h>

int sysc_read(int fd, void *buf, size_t count) {
    if (fd < 0 || fd >= NR_OPENS) return -EBADF;

    // only support char device
    // only support read from console.
    // ignore fd
    chrdev_t *p = chrdev[CHRDEV_CNSL];
    return p->read(buf, count);
}

/*
 *--------------------------------------------------------------------------
 *   File Name: write.c
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Sun Mar  8 11:05:12 2009
 * Last Update: Sun Mar  8 11:05:12 2009
 *
 *--------------------------------------------------------------------------
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fs.h>
#include <types.h>
ssize_t vfs_generic_file_write(file_t *file, const char *buf, size_t size, loff_t *p_pos) {
    ssize_t ret = 0;

    loff_t pos = *p_pos;
    if (buf == NULL || size < 0 || pos < 0) {
        return -EINVAL;
    }

    assert(file->f_dentry->d_inode != NULL);
    assert((file->f_flags & O_APPEND) == O_APPEND);  // 目前只支持这个

    inode_t *inode = file->f_dentry->d_inode;

    down(&inode->i_sem);

    while (size > 0) {
        uint32_t index = pos >> PAGE_SHIFT;
        uint32_t offset = pos & (PAGE_SIZE - 1);
        uint32_t bytes = PAGE_SIZE - offset;
        if (size < bytes) {
            bytes = size;
        }

        //
    }

end:
    up(&inode->i_sem);

    return ret;
}

ssize_t sysc_write(int fd, const char *buf, size_t size) {
    ssize_t ret = 0;

    file_t *file = get_file(fd);
    if (NULL == file) {
        return -EBADF;
    }

    // TODO 检查文件是否有写权限

    inode_t *inode = file->f_dentry->d_inode;

    assert(file->f_ops != 0);
    assert(file->f_ops->write != 0);
    assert(inode->i_fops != 0);
    assert(inode->i_fops->write != 0);
    assert(file->f_ops->write == inode->i_fops->write);

    ssize_t (*write)(file_t *, const char *, size_t, loff_t *);
    write = file->f_ops->write;

    ret = write(file, buf, size, &file->f_pos);

    // TODO 释放file的引用计数

    return ret;
}

// extern void vga_puts(unsigned int nr, const char *buf, unsigned char color);
// int sysc_write(int fd, const char *buf, unsigned long size) {
//     if (size < 0) return -1;

//     switch (fd) {
//     case 0:
//         assert(0);
//         // vga_puts(0, buf, 0xF);
//         break;
//     default:
//         return -1;
//     }

//     return size;
// }

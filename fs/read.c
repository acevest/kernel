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

ssize_t vfs_generic_file_read(file_t *file, char *buf, size_t size, loff_t *p_pos) {
    ssize_t ret = 0;

    loff_t pos = *p_pos;

    inode_t *inode = file->f_dentry->d_inode;
    address_space_t *mapping = inode->i_mapping;

    assert(S_ISREG(inode->i_mode));

    uint32_t index = pos >> PAGE_SHIFT;
    uint32_t offset = pos & (PAGE_SIZE - 1);
    size_t left = size;

    while (left > 0) {
        page_t *page = NULL;
        uint32_t end_index = inode->i_size >> PAGE_SHIFT;
        if (index > end_index) {
            break;
        }

        uint32_t bytes = PAGE_SIZE;
        if (index == end_index) {
            bytes = inode->i_size & (PAGE_SIZE - 1);
            if (bytes <= offset) {
                break;
            }
        }

        bytes = bytes - offset;

        // 在hash里找page
        page_t *find_hash_page(address_space_t * mapping, uint32_t index);
        page = find_hash_page(mapping, index);
        if (NULL == page) {
            goto no_cached_page_in_hash;
        }

        // 在hash里找到了page
        // TODO 增加page引用计数

        // copy data
        bytes = bytes < left ? bytes : left;
        void *addr = page2va(page);
        // printk("memcpy bytes %u index %u\n", bytes, index);
        // printk("read addr %x bytes %u index %u offset %u\n", addr, bytes, index, offset);
        memcpy(buf, addr + offset, bytes);

        buf += bytes;
        offset += bytes;
        index += offset >> PAGE_SHIFT;
        offset &= (PAGE_SIZE - 1);

        // TODO 减少page引用计数

        left -= bytes;
        if (bytes != 0 && left != 0) {
            continue;
        } else {
            break;
        }

    no_cached_page_in_hash:
        page = alloc_one_page(0);
        if (page == NULL) {
            return ENOMEM;
        }

        // TODO：可能已经有其它进程已经把数据读入内存了
        //

        // read page
        assert(mapping->a_ops != NULL);
        assert(mapping->a_ops->read_page != NULL);
        ret = mapping->a_ops->read_page(file, page);
        if (0 != ret) {
            return EIO;
        }

        //
        void add_page_to_hash(page_t * page, address_space_t * mapping, uint32_t index);
        add_page_to_hash(page, mapping, index);
    }

    return ret;
}

ssize_t sysc_read(int fd, void *buf, size_t count) {
    ssize_t ret = 0;

    file_t *file = get_file(fd);
    if (NULL == file) {
        return EBADF;
    }

    inode_t *inode = file->f_dentry->d_inode;

    assert(file->f_ops != 0);
    assert(file->f_ops->read != 0);
    assert(inode->i_fops != 0);
    assert(inode->i_fops->read != 0);
    assert(file->f_ops->read == inode->i_fops->read);

    ssize_t (*read)(file_t *, char *, size_t, loff_t *);
    read = file->f_ops->read;

    loff_t pos = file->f_pos;
    pos = 0;  // TODO add sysc_seek
    // printk("%s  pos %lu\n", file->f_dentry->d_inline_name, pos);
    ret = read(file, buf, count, &pos);

    return ret;
}

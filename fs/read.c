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
#include <types.h>
#include <assert.h>
#include <fs.h>
#include <errno.h>
#include <sched.h>

int sysc_read(int fd, void *buf, size_t count)
{
#if 0
    if(fd<0 || fd>=NR_OPENS)
        return -EBADF;

    pFile fp = current->fps[fd];
    assert(fp != NULL);

    pInode    inode = fp->inode;
    assert(inode->i_size > 0);    // 目前只能这样支持
    if(inode->i_size > MAX_SUPT_FILE_SIZE)
        return -EFAULT;


    return read_file(inode, buf, count);
#endif
}

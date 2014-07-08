/*
 *--------------------------------------------------------------------------
 *   File Name: stat.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Tue Feb 23 19:56:08 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <types.h>
#include <stat.h>
#include <errno.h>
#include <fs.h>
#include <sched.h>
#include <memory.h>
int sysc_stat(int fd, struct stat *stat)
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

    memset((void*)stat, 0, sizeof(struct stat));

    stat->st_ino    = fp->ino_nr;
    stat->st_mode    = inode->i_mode;
    stat->st_uid    = inode->i_uid;
    stat->st_gid    = inode->i_gid;
    stat->st_size    = inode->i_size;
    stat->st_blocks    = inode->i_blocks;
    stat->st_atime    = inode->i_atime;
    stat->st_mtime    = inode->i_mtime;
    stat->st_ctime    = inode->i_ctime;
#endif

    return 0;
}

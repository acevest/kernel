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

#include <fs.h>
#include <errno.h>
#include <types.h>
#include <sched.h>
#include <fcntl.h>
#include <assert.h>
#include <syscall.h>

int sysc_open(const char *path, int flags, mode_t mode)
{
    assert(mode == 0); // unsupport now...
    assert(flags == O_RDONLY);    // support only...

    int ino_nr, i, fd;
    pInode inode;

    /* 先获取文件的i节点号 */
    ino_nr = get_inode_nr(path);
    if(ino_nr == EXT2_BAD_INO)
        return -ENOENT;

    /* 找到空的文件描述符句柄 */
    for(i=0; i<NR_OPENS; i++)
        if(current->fps[i] == NULL)
            break;
    if(i == NR_OPENS)
        return -EMFILE;
    fd = i;


    /* 找到空的描述符或已经打开的描述符 */
    int empt_nr, fdt_nr;
    pFile pf;
    empt_nr = fdt_nr = -1;
    for(i=0, pf=file_table; i<NR_FILES; i++, pf++)
    {
        if(pf->ino_nr == ino_nr)
        {
            fdt_nr = i;
            break;
        }
        else if(pf->ino_nr == 0)
        {
            empt_nr = i;
        }
    }

    if(fdt_nr != -1)
    {
        pf = file_table+fdt_nr;
    }
    else if(empt_nr != -1)
    {
        pf = file_table+empt_nr;
    }
    else
    {
        return -EMFILE;
    }


    if(pf->ino_nr == ino_nr)
    {
        pf->count++;
        current->fps[fd] = pf;
        return fd;
    }

    inode = find_empty_inode();
    if(inode == NULL)
        return -ENOMEM;

    get_inode(ino_nr, inode);
    
    pf->count = 1;
    pf->ino_nr = ino_nr;
    pf->inode = inode;
    current->fps[fd] = pf;

    return fd;
}

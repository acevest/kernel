/*
 *--------------------------------------------------------------------------
 *   File Name:	test.c
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 			Tue Feb 23 17:47:43 2010
 * 
 * Description:	sysc_test
 * 
 *--------------------------------------------------------------------------
 */
#include<syscall.h>
#include<printk.h>
#include<sched.h>

void dump_fd()
{
	int i;
	for(i=0; i<NR_OPENS; i++)
	{
		pFile pf = current->fps[i];
		if(pf == NULL) continue;
		printk("fd[%d]: %08x\n", i, pf);


		pInode inode = pf->inode;
		printk("file_desc ino_nr: %04d inode:%08x\n",
			pf->ino_nr, inode);

		printk("inode: size: %d\n", inode->i_size);
	}
}

int sysc_test()
{
	dump_fd();

	return 0;
}

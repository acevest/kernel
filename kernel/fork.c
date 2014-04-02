/*
 *--------------------------------------------------------------------------
 *   File Name:	fork.c
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 			Sun Feb  7 13:25:28 2010
 * 
 * Description:	none
 * 
 *--------------------------------------------------------------------------
 */

#include<sched.h>
#if 1
int sysc_fork()
{
	// 先分配一个进程控制结构
	task_struct *tsk;
	// TODO tsk = get_unused_task_pcb();
	if(tsk == NULL)
		panic("can not malloc PCB");

	//printk("CHILD:%08x\n", tsk);

	memcpy(tsk, current, sizeof(task_struct));

	tsk->pid	= get_next_pid();
	tsk->ppid	= current->pid;

	init_tsk_cr3(tsk);

	int i, j;
	u32 *p_pd  = (u32 *) current->cr3; // parent's page dir
	u32 p_pde; // parent's page dir entry
	u32 *p_pt  = NULL;
	u32 *c_pd  = (u32 *) tsk->cr3;
	u32 c_pde;
	u32 *c_pt  = NULL;

	p_pd = pa2va(p_pd);
	c_pd = pa2va(c_pd);

	for(i = (KRNLADDR>>22); i>=0; i--)
	{
		p_pde = p_pd[i] & 0xFFFFF000;
		if(p_pde == 0)
			continue;

		//printk("i:%d p_pde:%08x ", i, p_pde);

		// 分配页表
		c_pde = (u32) get_phys_pages(1);

		//printk("c_pde:%08x ", c_pde);

		c_pt  = pa2va(c_pde);
		p_pt  = pa2va(p_pde);

		//printk("c_pt:%08x p_pt:%08x\n", c_pt, p_pt);

		for(j=0; j<PAGE_ITEMS; j++)
		{
			p_pt[j] &= (~2UL);
			c_pt[j] = p_pt[j];
			/* 增加此页的共享计数 */
			pgmap[p_pt[j]>>PAGE_SHIFT].count++;
		}

		c_pd[i] = c_pde | 7;
	}


	load_cr3(current);

	//pPtRegs	regs	= ((pPtRegs)(TASK_SIZE+(unsigned long) current))-1;
	//tsk->regs	= *regs;
	//tsk->regs.eax	= 0x00;
	//tsk->regs.eflags |= 0x200; //enable IF
	pPtRegs	regs	= ((pPtRegs)(TASK_SIZE+(unsigned long) tsk))-1;
	extern	void ret_from_fork();
	regs->eax	= 0x00;
	tsk->esp0	= TASK_SIZE + (unsigned long) tsk;
	tsk->esp	= (unsigned long) regs;
	tsk->eip	= (unsigned long) ret_from_fork;


	//printk("FORK:%08x\n", tsk);

    tsk->state = TASK_RUNNING;

	return (int)tsk->pid;
}
#endif

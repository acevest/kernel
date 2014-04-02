/*
 *--------------------------------------------------------------------------
 *   File Name:	hd.h
 * 
 * Description:	none
 * 
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:	1.0
 * Create Date: Tue Feb 10 15:17:57 2009
 * Last Update: Mon Feb 01 21:07:31 2010
 * 
 *--------------------------------------------------------------------------
 */

#ifndef	_HD_H
#define	_HD_H

#include<types.h>
#include<printk.h>
#include<assert.h>

#if 0
#define	HD_CHL0_DATA		0x1F0
#define	HD_CHL0_ERR		0x1F1
#define	HD_CHL0_NSECTOR	0x1F2
#define	HD_CHL0_SECTOR	0x1F3
#define	HD_CHL0_CYLL		0x1F4
#define	HD_CHL0_CYLH		0x1F5
#define	HD_CHL0_CURRENT	0x1F6	/*101nhhhh
				 n:0--First Hardisk, 1--Second Hardisk.
				 hhhh: head*/
#define	HD_STATUS	0x1F7
#define	HD_CMD		HD_STATUS
#endif


/* 命令寄存器的起始值 */
#define	HD_CHL0_CMD_BASE	0x1F0
#define	HD_CHL1_CMD_BASE	0x170

#define	HD_DATA			0
#define	HD_ERR			1
#define		HD_ERR_BB		0x80
#define		HD_ERR_ECC		0x40
#define		HD_ERR_ID		0x10
#define		HD_ERR_AC		0x04
#define		HD_ERR_TK		0x02
#define		HD_ERR_DM		0x01
#define	HD_NSECTOR		2
#define	HD_LBAL			3
#define	HD_LBAM			4
#define	HD_LBAH			5
#define	HD_DEVICE		6
#define	HD_STATUS		7		/* controller status */
#define		HD_STATUS_BSY		0x80	/* controller busy */
#define		HD_STATUS_RDY		0x40	/* drive ready */
#define		HD_STATUS_WF		0x20	/* write fault */
#define		HD_STATUS_SC		0x10	/* seek complete */
#define		HD_STATUS_DRQ		0x08	/* data transfer request */
#define		HD_STATUS_CRD		0x04	/* correct data */
#define		HD_STATUS_IDX		0x02	/* index pulse */
#define		HD_STATUS_ERR		0x01	/* error */
#define	HD_FEATURES		HD_ERR
#define	HD_CMD			HD_STATUS
#define		HD_CMD_IDLE		0x00
#define		HD_CMD_RECALIBRATE	0x10
#define		HD_CMD_READ		0x20	/* read data */
#define		HD_CMD_READ_EXT		0x24	/* read data (LBA-48 bit)*/
#define		HD_CMD_WRITE		0x30
#define		HD_CMD_WRITE_EXT	0x34
#define		HD_CMD_READ_VERIFY	0x40
#define		HD_CMD_FORMAT		0x50
#define		HD_CMD_SEEK		0x70
#define		HD_CMD_DIAG		0x90
#define		HD_CMD_SPECIFY		0x91
#define		HD_CMD_IDENTIFY		0xEC

/* 硬盘控制寄存器 */
#define	HD_CHL0_CTL_BASE	0x3F6
#define	HD_CHL1_CTL_BASE	0x376

#define	HD_CTL			0
#define		HD_CTL_NORETRY		0x80	/* disable access retry */
#define		HD_CTL_NOECC		0x40	/* disable ecc retry */
#define		HD_CTL_EIGHTHEADS	0x08	/* more than 8 heads */
#define		HD_CTL_RESET		0x04	/* reset controller */
#define		HD_CTL_DISABLE_INT	0x02	/* disable interrupts */

#define		HD_GET_CHL(dev)		(0)	/* 暂时还是只支持通道0 */
#define		HD_GET_DEV(dev)		(0)	/* 暂时还是只支持一个硬盘 */

#define	REG_CMD_BASE(dev, offset)			\
	(HD_GET_CHL(dev)?(HD_CHL1_CMD_BASE+offset)	\
	:						\
	(HD_CHL0_CMD_BASE+offset))

#define	REG_CTL_BASE(dev, offset)			\
	(HD_GET_CHL(dev)?(HD_CHL1_CTL_BASE+offset)	\
	:						\
	(HD_CHL0_CTL_BASE+offset))

#if 1
#define	REG_DATA(dev)		REG_CMD_BASE(dev, HD_DATA)
#define	REG_ERR(dev)		REG_CMD_BASE(dev, HD_ERR)
#define	REG_NSECTOR(dev)	REG_CMD_BASE(dev, HD_NSECTOR)
#define	REG_LBAL(dev)		REG_CMD_BASE(dev, HD_LBAL)
#define	REG_LBAM(dev)		REG_CMD_BASE(dev, HD_LBAM)
#define	REG_LBAH(dev)		REG_CMD_BASE(dev, HD_LBAH)
#define	REG_DEVICE(dev)		REG_CMD_BASE(dev, HD_DEVICE)
#define	REG_STATUS(dev)		REG_CMD_BASE(dev, HD_STATUS)
#define	REG_FEATURES(dev)	REG_CMD_BASE(dev, HD_FEATURES)
#define	REG_CMD(dev)		REG_CMD_BASE(dev, HD_CMD)

#define	REG_CTL(dev)		REG_CTL_BASE(dev, HD_CTL)
#else	
#define	REG_DATA	REG_CMD_BASE(0, HD_DATA)
#define	REG_ERR		REG_CMD_BASE(0, HD_ERR)
#define	REG_NSECTOR	REG_CMD_BASE(0, HD_NSECTOR)
#define	REG_LBAL	REG_CMD_BASE(0, HD_LBAL)
#define	REG_LBAM	REG_CMD_BASE(0, HD_LBAM)
#define	REG_LBAH	REG_CMD_BASE(0, HD_LBAH)
#define	REG_DEVICE	REG_CMD_BASE(0, HD_DEVICE)
#define	REG_STATUS	REG_CMD_BASE(0, HD_STATUS)
#define	REG_FEATURES	REG_CMD_BASE(0, HD_FEATURES)
#define	REG_CMD		REG_CMD_BASE(0, HD_CMD)

#define	REG_CTL		REG_CTL_BASE(0, HD_CTL)
#endif

/* 一个扇区包含的字节数 */
#define	SECT_SIZE	512


static inline void hd_rd_port(int port, void *buf, unsigned long count)
{
	unsigned char *p =(unsigned char *)buf;
	unsigned char value[4];
	int i, n;
	n = (count & 3UL);
	if(n == 0)
	{
		insl(port, p, count>>2);
	}
	else
	{
		insl(port, p, (count>>2));
		insl(port, value, 1);

		count -= n;
		for(i=0; i<n; i++)
			p[count+i] = value[i];
	}

#if 0	
	unsigned char *p =(unsigned char *)buf;
	int l, w, b;
	l = count & (~3UL);
	w = count - l;
	b = w & 0x01;
	w = w - b;
	printk("l:%d w:%d b:%d\n", l, w, b);
	if(l)
	{
		insl(port, p, l);
		p += l;
	}
	if(w)
	{
		insw(port, p+l, w);
		p += w;
	}
	if(b)
	{
		insb(port, p+l+w, b);
	}
#endif
#if 0
	int i, rest;
	unsigned char *p =(unsigned char *)buf;

	rest = count & 3UL;
	count-= rest;

	printk("count:%d rest:%d\n", count, rest);

	insl(port, p, count>>2);

	for(i=0; i<rest; i++)
		insb(port, p+count+i, 1);
#endif
#if 0
	if(!(count & 0x3UL))
	{
		count >>= 2;
		insl(port, buf, count-2);
	}
	else if(!(count & 0x1UL))
	{
		count >>= 1;
		insw(port, buf, count);
	}
#endif
}

/* 从硬盘数据端口读取数据 */
#define	hd_rd_data(dev, buf, count) hd_rd_port(REG_DATA(dev), buf, count)

#define	hd_bsy(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_BSY))
#define	hd_rdy(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_RDY))
#define	hd_drq(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_DRQ))
#define	hd_err(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_ERR))


#define	USE_LBA_48	/* 使用LBA-48bit 模式操作硬盘 */

#endif //_HD_H

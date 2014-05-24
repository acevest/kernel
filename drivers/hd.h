/*
 *--------------------------------------------------------------------------
 *   File Name: hd.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Tue Feb 10 15:17:57 2009
 * Last Update: Mon Feb 01 21:07:31 2010
 * 
 *--------------------------------------------------------------------------
 */

#ifndef _HD_H
#define _HD_H

#include <types.h>
#include <printk.h>
#include <assert.h>

/* hard disk command base register */
#define HD_CHL0_CMD_BASE    0x1F0
#define HD_CHL1_CMD_BASE    0x170

#define HD_DATA         0
#define HD_FEATURES     1
#define HD_ERR          1
#define     HD_ERR_BB        0x80
#define     HD_ERR_ECC       0x40
#define     HD_ERR_ID        0x10
#define     HD_ERR_AC        0x04
#define     HD_ERR_TK        0x02
#define     HD_ERR_DM        0x01
#define HD_NSECTOR      2
#define HD_LBAL         3
#define HD_LBAM         4
#define HD_LBAH         5
#define HD_DEVSEL       6
#define HD_CMD          7
#define HD_STATUS       7        /* controller status */
#define     HD_STATUS_BSY       0x80    /* controller busy */
#define     HD_STATUS_RDY       0x40    /* drive ready */
#define     HD_STATUS_WF        0x20    /* write fault */
#define     HD_STATUS_SEEK_CMPT 0x10    /* seek complete */
#define     HD_STATUS_DRQ       0x08    /* data transfer request */
#define     HD_STATUS_CRD       0x04    /* correct data */
#define     HD_STATUS_IDX       0x02    /* index pulse */
#define     HD_STATUS_ERR       0x01    /* error */
#define     HD_CMD_IDLE         0x00
#define     HD_CMD_RECALIBRATE  0x10
#define     HD_CMD_READ         0x20    /* read data */
#define     HD_CMD_READ_EXT     0x24    /* read data (LBA-48 bit)*/
#define     HD_CMD_READ_DMA     0x25    /* read data DMA LBA48 */
#define     HD_CMD_WRITE        0x30
#define     HD_CMD_WRITE_EXT    0x34
#define     HD_CMD_READ_VERIFY  0x40
#define     HD_CMD_FORMAT       0x50
#define     HD_CMD_SEEK         0x70
#define     HD_CMD_DIAG         0x90
#define     HD_CMD_SPECIFY      0x91
#define     HD_CMD_IDENTIFY     0xEC

/* hard disk control register */
#define HD_CHL0_CTL_BASE    0x3F6
#define HD_CHL1_CTL_BASE    0x376


#define HD_CTL            0
#define     HD_CTL_NORETRY      0x80    /* disable access retry */
#define     HD_CTL_NOECC        0x40    /* disable ecc retry */
#define     HD_CTL_EIGHTHEADS   0x08    /* more than 8 heads */
#define     HD_CTL_RESET        0x04    /* reset controller */
#define     HD_CTL_DISABLE_INT  0x02    /* disable interrupts */

#define     HD_GET_CHL(dev)     (0)     /* only support channel 0 */
#define     HD_GET_DEV(dev)     (0)     /* only support one hard disk */

#define REG_CMD_BASE(dev, offset)  ( HD_GET_CHL(dev) ? (HD_CHL1_CMD_BASE+offset) : (HD_CHL0_CMD_BASE+offset) )
#define REG_CTL_BASE(dev, offset)  ( HD_GET_CHL(dev) ? (HD_CHL1_CTL_BASE+offset) : (HD_CHL0_CTL_BASE+offset) )

#define REG_DATA(dev)       REG_CMD_BASE(dev, HD_DATA)
#define REG_ERR(dev)        REG_CMD_BASE(dev, HD_ERR)
#define REG_NSECTOR(dev)    REG_CMD_BASE(dev, HD_NSECTOR)
#define REG_LBAL(dev)       REG_CMD_BASE(dev, HD_LBAL)
#define REG_LBAM(dev)       REG_CMD_BASE(dev, HD_LBAM)
#define REG_LBAH(dev)       REG_CMD_BASE(dev, HD_LBAH)
#define REG_DEVSEL(dev)     REG_CMD_BASE(dev, HD_DEVSEL)
#define REG_STATUS(dev)     REG_CMD_BASE(dev, HD_STATUS)
#define REG_FEATURES(dev)   REG_CMD_BASE(dev, HD_FEATURES)
#define REG_CMD(dev)        REG_CMD_BASE(dev, HD_CMD)
#define REG_CTL(dev)        REG_CTL_BASE(dev, HD_CTL)

#define SECT_SIZE    512


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
}

#define hd_rd_data(dev, buf, count) hd_rd_port(REG_DATA(dev), buf, count)

#define hd_bsy(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_BSY))
#define hd_rdy(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_RDY))
#define hd_drq(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_DRQ))
#define hd_err(dev) ((inb(REG_STATUS(dev)) & HD_STATUS_ERR))

#endif //_HD_H

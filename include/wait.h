/*
 *--------------------------------------------------------------------------
 *   File Name:	wait.h
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 			Mon Feb 22 20:50:56 2010
 * 
 * Description:	none
 * 
 *--------------------------------------------------------------------------
 */

#ifndef	_WAIT_H
#define	_WAIT_H

#include<list.h>

typedef	struct
{
	ListHead wait;
} WaitQueueHead, *pWaitQueueHead;

typedef	ListHead WaitQueue, *pWaitQueue;

void init_wait_queue(pWaitQueueHead wqh);


#endif //_WAIT_H

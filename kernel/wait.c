/*
 *--------------------------------------------------------------------------
 *   File Name: wait.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Mon Feb 22 20:45:22 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <wait.h>

void init_wait_queue(pWaitQueueHead wqh)
{
    INIT_LIST_HEAD(&wqh->wait);
}

void add_wait_queue(pWaitQueueHead wqh, pWaitQueue wq)
{
    list_add_tail(wq, &wqh->wait);
}

void del_wait_queue(pWaitQueueHead wqh, pWaitQueue old)
{
    //list_del_init();
}

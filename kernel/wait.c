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

void init_wait_queue(wait_queue_head_t * wqh)
{
    INIT_LIST_HEAD(&wqh->wait);
}

void add_wait_queue(wait_queue_head_t * wqh, wait_queue_t * wq)
{
    list_add_tail(wq, &wqh->wait);
}

void del_wait_queue(wait_queue_head_t * wqh, wait_queue_t * old)
{
    //list_del_init();
}

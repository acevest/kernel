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

void init_wait_queue(wait_queue_head_t *wqh)
{
    INIT_LIST_HEAD(&wqh->task_list);
}

void add_wait_queue(wait_queue_head_t *wqh, wait_queue_t *wq)
{
    unsigned long iflags;
    irq_save(iflags);
    list_add_tail(&wq->task_list, &wqh->task_list);
    irq_restore(iflags);
}

void del_wait_queue(wait_queue_head_t *wqh, wait_queue_t *wq)
{
    unsigned long iflags;
    irq_save(iflags);
    list_del(&wq->task_list);
    irq_restore(iflags);
}


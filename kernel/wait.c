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
    unsigned long flags;
    irq_save(flags);
    list_add_tail(&wq->task_list, &wqh->task_list);
    irq_restore(flags);
}

void del_wait_queue(wait_queue_head_t *wqh, wait_queue_t *wq)
{
    unsigned long flags;
    irq_save(flags);
    list_del(&wq->task_list);
    irq_restore(flags);
}

void wake_up(wait_queue_head_t *wqh)
{
    unsigned long flags;
    wait_queue_t *p, *tmp;
    irq_save(flags);
    list_for_each_entry_safe(p, tmp, &wqh->task_list, task_list)
    {
        p->task->state = TASK_RUNNING;
    }
    irq_restore(flags);

    // no schedule() here.
}


#include<irq.h>
DECLARE_WAIT_QUEUE_HEAD(debug_wq);
unsigned int debug_global_var = 0;
int debug_wait_queue_get()
{
    unsigned int v = 0;
    task_union * task = current;
    DECLARE_WAIT_QUEUE(wait, task);
    add_wait_queue(&debug_wq, &wait);

    while(1)
    {
        printk("pid %d is going to wait\n", sysc_getpid());
        task->state = TASK_WAIT;

        disable_irq();
        v = debug_global_var;
        if(debug_global_var != 0)
            debug_global_var--;
        enable_irq();

        if(v != 0)
            break;

        schedule();
        printk("pid %d is running\n", sysc_getpid());
    }

    printk("pid %d is really running\n", sysc_getpid());
    task->state = TASK_RUNNING;
    del_wait_queue(&debug_wq, &wait);

    return v;
}

int debug_wait_queue_put(unsigned int v)
{
    debug_global_var = v;
    wake_up(&debug_wq);
}

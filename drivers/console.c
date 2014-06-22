/*
 * ------------------------------------------------------------------------
 *   File Name: console.c
 *      Author: Zhao Yanbai
 *              Sun Jun 22 18:50:13 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#include<string.h>
#include<console.h>

cnsl_queue_t cnsl_rd_q;
cnsl_queue_t cnsl_wr_q;
cnsl_queue_t cnsl_sc_q;

static void cnsl_queue_init(cnsl_queue_t *cq)
{
    memset((void *)cq, 0, sizeof(*cq));

    cq->head = 0;
    cq->tail = 0;
    init_wait_queue(&cq->wait);
    //cq->data = kmalloc(CNSL_QUEUE_SIZE, 0);

    printk("console queue data addr %08x\n", cq->data);
}

void cnsl_init()
{
    cnsl_queue_init(&cnsl_rd_q);
    cnsl_queue_init(&cnsl_wr_q);
    cnsl_queue_init(&cnsl_sc_q);
}

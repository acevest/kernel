/*
 * ------------------------------------------------------------------------
 *   File Name: console.h
 *      Author: Zhao Yanbai
 *              Sun Jun 22 22:06:53 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include<wait.h>

#define CNSL_QUEUE_SIZE 1024

typedef struct cnsl_queue
{
    unsigned int head;
    unsigned int tail;
    wait_queue_head_t wait;
    char data[CNSL_QUEUE_SIZE];
} cnsl_queue_t;

extern cnsl_queue_t cnsl_rd_q;
extern cnsl_queue_t cnsl_wr_q;
extern cnsl_queue_t cnsl_sc_q;

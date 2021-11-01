/*
 * ------------------------------------------------------------------------
 *   File Name: console.h
 *      Author: Zhao Yanbai
 *              Sun Jun 22 22:06:53 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <wait.h>

#define CNSL_QUEUE_SIZE 1024

typedef struct cnsl_queue
{
    unsigned int head;
    unsigned int tail;
    wait_queue_head_t wait;
    char data[CNSL_QUEUE_SIZE];
} cnsl_queue_t;

typedef struct cnsl
{
    cnsl_queue_t rd_q;
    cnsl_queue_t wr_q;
    cnsl_queue_t sc_q;
} cnsl_t;

int cnsl_kbd_write(char c);

/*
 *--------------------------------------------------------------------------
 *   File Name: wait.h
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Mon Feb 22 20:50:56 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */

#pragma once

#include <list.h>

typedef struct
{
    list_head_t wait;

} wait_queue_head_t;

typedef list_head_t wait_queue_t;

void init_wait_queue(wait_queue_head_t * wqh);

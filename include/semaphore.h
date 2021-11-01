/*
 * ------------------------------------------------------------------------
 *   File Name: semaphore.h
 *      Author: Zhao Yanbai
 *              Sun Jun 22 13:53:18 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <list.h>
#include <task.h>
#include <irq.h>

typedef struct semaphore
{
    volatile unsigned int cnt;
    list_head_t wait_list;
} semaphore_t;

#define SEMAPHORE_INITIALIZER(name, n)                \
    {                                                 \
        .cnt = (n),                                   \
        .wait_list = LIST_HEAD_INIT((name).wait_list) \
    }

#define DECLARE_MUTEX(name) \
    semaphore_t name = SEMAPHORE_INITIALIZER(name, 1)

void down(semaphore_t *s);
void up(semaphore_t *s);

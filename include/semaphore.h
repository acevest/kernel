/*
 * ------------------------------------------------------------------------
 *   File Name: semaphore.h
 *      Author: Zhao Yanbai
 *              Sun Jun 22 13:53:18 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <irq.h>
#include <list.h>
#include <task.h>

typedef struct semaphore {
    volatile unsigned int cnt;
    list_head_t wait_list;
} semaphore_t;

#define SEMAPHORE_INITIALIZER(name, n) \
    { .cnt = (n), .wait_list = LIST_HEAD_INIT((name).wait_list) }

void semaphore_init(semaphore_t *s, unsigned int v);

void down(semaphore_t *s);
void up(semaphore_t *s);

#define DECLARE_MUTEX(name) semaphore_t name = SEMAPHORE_INITIALIZER(name, 1)
void mutex_lock(semaphore_t *);
void mutex_unlock(semaphore_t *);
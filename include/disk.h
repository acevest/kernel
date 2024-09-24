/*
 * ------------------------------------------------------------------------
 *   File Name: disk.h
 *      Author: Zhao Yanbai
 *              2021-11-21 22:08:16 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <buffer.h>
#include <completion.h>
#include <fs.h>
#include <list.h>
#include <semaphore.h>

typedef enum {
    DISK_REQ_IDENTIFY,
    DISK_REQ_READ,
} disk_request_cmd_t;

typedef struct disk_request {
    dev_t dev;
    uint64_t pos;    // 扇区号
    uint16_t count;  // 扇区数
    void *buf;       // 到的缓冲区
    bbuffer_t *bb;
    disk_request_cmd_t command;  // 命令
    list_head_t list;
    semaphore_t sem;

    int ret;
} disk_request_t;

typedef struct {
    uint32_t count;
    list_head_t list;

    // 供disk任务睡眠和被唤醒用
    semaphore_t sem;
} disk_request_queue_t;

int send_disk_request(disk_request_t *r);

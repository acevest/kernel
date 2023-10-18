/*
 * ------------------------------------------------------------------------
 *   File Name: disk.h
 *      Author: Zhao Yanbai
 *              2021-11-21 22:08:16 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <completion.h>
#include <list.h>
#include <semaphore.h>
#include <types.h>

typedef enum {
    DISK_REQ_IDENTIFY,
    DISK_REQ_READ,
} disk_request_cmd_t;

typedef struct disk_request {
    dev_t dev;
    uint64_t pos;                // 扇区号
    uint16_t count;              // 扇区数
    void *buf;                   // 到的缓冲区
    disk_request_cmd_t command;  // 命令
    list_head_t list;
    semaphore_t sem;
} disk_request_t;

typedef struct {
    uint32_t count;
    list_head_t list;

    // 供disk任务睡眠和被唤醒用
    semaphore_t sem;
} disk_request_queue_t;

void send_disk_request(disk_request_t *r);

void ide_disk_read(dev_t dev, uint32_t sect_nr, uint32_t count, char *buf);

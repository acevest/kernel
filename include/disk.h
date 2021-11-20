/*
 * ------------------------------------------------------------------------
 *   File Name: disk.h
 *      Author: Zhao Yanbai
 *              2021-11-21 22:08:16 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include <list.h>
#include <semaphore.h>
#include <types.h>
#include <wait.h>

typedef enum {
    DISK_REQ_IDENTIFY,
    DISK_REQ_READ,
} disk_request_cmd_t;

typedef struct disk_request {
    uint64_t pos;                // 扇区号
    uint16_t count;              // 扇区数
    void *buf;                   // 到的缓冲区
    disk_request_cmd_t command;  // 命令
    list_head_t list;
    wait_queue_head_t wait;  // 等待队列
    // 驱动器完全有可能在进程在进程睡眠到等待队列前返回数据并执行唤醒操作
    // 这时等待队列上无进程，就相当于不执行任何操作
    // 然后进程再睡眠到等待队列上，就会造成永远无法唤醒该进程
    // 因此要添加一个字段，标志驱动器已经对该请求做过唤醒操作
    // 进程在睡眠前需要检查该字段
    int done;
} disk_request_t;

typedef struct {
    uint32_t count;
    semaphore_t sem;
    list_head_t list;
} disk_request_queue_t;

void send_disk_request(disk_request_t *r);
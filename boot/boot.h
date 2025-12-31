/*
 * ------------------------------------------------------------------------
 *   File Name: boot.h
 *      Author: Zhao Yanbai
 *              Sat Mar 29 09:02:21 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include "multiboot2.h"

#define BOOT_INIT_PAGETBL_CNT 2  // 8MB

#ifndef ASM
#include <types.h>

#define E820_RAM 1
#define E820_RESERVED 2
#define E820_ACPI 3
#define E820_NVS 4
#define E820_UNUSABLE 5

#define E820_MAP_CNT 128

struct e820_entry {
    uint64_t addr;
    uint64_t size;
    uint32_t type;
};

struct e820map {
    unsigned long map_cnt;
    struct e820_entry map[E820_MAP_CNT];
};

struct boot_params {
    char cmdline[256];
    char bootloader[64];

    unsigned long root_device;  // 从cmdline里解析出来的

    unsigned long mem_lower;  // in bytes
    unsigned long mem_upper;

    void* boot_module_begin;
    void* boot_module_end;

    unsigned long biosdev;
    unsigned long partition;
    unsigned long sub_partition;

    struct e820map e820map;
};

typedef struct bootmem_data {
    unsigned long min_pfn;
    unsigned long max_pfn;

    // 准备分配的超始pfn
    unsigned long prepare_alloc_pfn;

    void* bitmap;
    unsigned long mapsize;
} bootmem_data_t;

extern struct boot_params boot_params;
extern bootmem_data_t bootmem_data;

#endif

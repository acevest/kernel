/*
 * ------------------------------------------------------------------------
 *   File Name: boot.h
 *      Author: Zhao Yanbai
 *              Sat Mar 29 09:02:21 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include "multiboot.h"

#define BOOT_INIT_PAGETBL_CNT 2

#ifndef ASM

#define E820_RAM 1
#define E820_RESERVED 2
#define E820_ACPI 3
#define E820_NVS 4
#define E820_UNUSABLE 5

#define E820_MAP_CNT 128

struct e820_entry
{
    unsigned long addr;
    unsigned long size;
    unsigned long type;
};

struct e820map
{
    unsigned long map_cnt;
    struct e820_entry map[E820_MAP_CNT];
};

struct boot_params
{
    char *cmdline;
    unsigned long boot_device;
    unsigned long root_device;

    unsigned long mem_lower; // in bytes
    unsigned long mem_upper;

    struct e820map e820map;
};

typedef struct bootmem_data
{
    unsigned long min_pfn;
    unsigned long max_pfn;

    unsigned long last_offset;  // offset to pfn2pa(this->min_pfn);
    unsigned long last_hit_pfn; // last hit index in bitmap

    void *bitmap;
    unsigned long mapsize;
} bootmem_data_t;

extern struct boot_params boot_params;
extern bootmem_data_t bootmem_data;

#endif

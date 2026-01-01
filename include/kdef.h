/*
 * ------------------------------------------------------------------------
 *   File Name: kdef.h
 *      Author: Zhao Yanbai
 *              2026-01-01 13:02:43 Thursday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#define KERNEL_VADDR_BASE (0xC0000000)

// 固定映射区划分出16MB的空间
#define FIX_MAP_VADDR_SIZE (16 << 20)

// 定义最大显存为 16MB
// 后续内核不映射显存了，以后可以提供映射到用户空间的功能，由用户态程序操作
#define VRAM_VADDR_SIZE (16 << 20)

// 最大支持的线性地址空间为1G
// 这里这样实现是为了简单，与Linux内核实现有显著不同
// Linux内核还把内存大致分为了 DMA(<16MB), NORMAL(<896MB), HIGHMEM(896~4G)的内存分区(zone)
// 并且每个分区各一个伙伴算法管理物理页。
// 正常Linux内核只有1G的寻址空间，正常只能寻址896MB以下的内存，余下的128MB地址空间是留着用来映射高端区域的物理内存的
#define MAX_SUPT_VADDR_SIZE (1UL << 30)

// 把内核线性地址的最高部分留给显存
// 余下的部分为支持映射其它物理内存的空间
#define MAX_SUPT_PHYMM_SIZE (MAX_SUPT_VADDR_SIZE - VRAM_VADDR_SIZE - FIX_MAP_VADDR_SIZE)

// 算出显存的线性地址
// 之后要将这个地址映射到显存的物理地址
#define VRAM_VADDR_BASE (PAGE_OFFSET + MAX_SUPT_PHYMM_SIZE)
#define VRAM_VADDR_END (VRAM_VADDR_BASE + VRAM_VADDR_SIZE)

// 算出固定映射区的线性地址
#define FIX_MAP_VADDR_BASE (VRAM_VADDR_END)
#define FIX_MAP_VADDR_END (FIX_MAP_VADDR_BASE + FIX_MAP_VADDR_SIZE)

/*
 * ------------------------------------------------------------------------
 *   File Name: mount.h
 *      Author: Zhao Yanbai
 *              2024-08-30 21:40:51 Friday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

#include "vfs.h"

vfsmount_t* vfs_kernel_mount(fs_type_t* type, int flags, const char* name, void* data);

/*
 *--------------------------------------------------------------------------
 *   File Name: stdio.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Mon Mar  9 01:59:06 2009
 * Last Update: Mon Mar  9 01:59:06 2009
 *
 *--------------------------------------------------------------------------
 */

#ifndef _STDIO_H
#define _STDIO_H
#include <string.h>
#include <syscall.h>
extern int write(int fd, const char* buf, unsigned long size);
extern int vsprintf(char* buf, const char* fmt, char* args);
static inline int printf(const char* fmt, ...) {
    char ptfbuf[512];
    char* args = (char*)(((char*)&fmt) + 4);
    vsprintf(ptfbuf, fmt, args);
    return write(0, ptfbuf, strlen(ptfbuf));
}

#endif  //_STDIO_H

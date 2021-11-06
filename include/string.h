/*
 *--------------------------------------------------------------------------
 *   File Name: string.h
 *
 * Description: none
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Wed Jul 30 16:03:27 2008
 * Last Update: Wed Jul 30 16:03:27 2008
 *
 *--------------------------------------------------------------------------
 */

#ifndef _STRING_H
#define _STRING_H

#include "types.h"

char *strcpy(char *dest, const char *src);
char *strncpy(char *dst, const char *src, size_t len);
size_t strlcpy(char *dst, const char *src, size_t size);
size_t strlen(const char *str);
int strcmp(const char *a, const char *b);
int strncmp(const char *a, const char *b, size_t count);
char *strcat(char *dest, const char *src);
char *strstr(const char *a, const char *b);

void *memcpy(void *dest, const void *src, size_t size);
void memset(void *dest, char ch, size_t size);
int memcmp(const void *a, const void *b, size_t count);

#endif  //_STRING_H

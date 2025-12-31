/*
 *--------------------------------------------------------------------------
 *   File Name: assert.h
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sat Jan 23 14:02:00 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */

#ifndef _ASSERT_H
#define _ASSERT_H

#include <global.h>

#ifndef ASM
void assert_fail(char* exp, char* file, unsigned int line, const char* func);
#define assert(exp) ((exp) ? (void)(0) : assert_fail(__STRING(exp), __FILE__, __LINE__, __PRETTY_FUNCTION__))
#endif

#endif  //_ASSERT_H

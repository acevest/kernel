/*
 * ------------------------------------------------------------------------
 *   File Name: wait.c
 *      Author: Zhao Yanbai
 *              Sun Jul 27 19:25:38 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <syscall.h>
int wait(unsigned long pid) { syscall1(SYSC_WAIT, pid); }

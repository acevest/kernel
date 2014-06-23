/*
 *--------------------------------------------------------------------------
 *   File Name: printk.h
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Wed Mar  4 21:49:19 2009
 * Last Update: Wed Mar  4 21:49:19 2009
 * 
 *--------------------------------------------------------------------------
 */

#pragma once

int printk(char *fmtstr, ...);
int printd(unsigned int line, const char *fmtstr, ...);

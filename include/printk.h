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

void switch_printk_screen();
int printk(const char *fmtstr, ...);
int printd(const char *fmtstr, ...);
int printlo(unsigned int line, unsigned int offset, const char *fmtstr, ...);

#define printl(line, fmt, args...) printlo(1, line, fmt, ##args)
#define printll(line, fmt, args...) printlo(0, line, fmt, ##args)
#define printlr(line, fmt, args...) printlo(40, line, fmt, ##args)

// monitor print line
enum {
    MPL_TITLE,
    MPL_ROOTDEV,
    MPL_CLOCK,
    MPL_KEYBOARD,
    MPL_IDE,
    MPL_IDE_INTR,
    MPL_PREEMPT,
    MPL_TEST,
    MPL_DEBUG,
    MPL_ROOT,
    MPL_TASK_0,
    MPL_TASK_1,
    MPL_TASK_2,
    MPL_TASK_3,
    MPL_TASK_4,
    MPL_TASK_5,
    MPL_TASK_6,
    MPL_TASK_7,
    MPL_TASK_8,
    MPL_END
};

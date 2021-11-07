/*
 * ------------------------------------------------------------------------
 *   File Name: tty.h
 *      Author: Zhao Yanbai
 *              2021-11-07 17:17:23 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#pragma once

typedef struct tty {
    char name[32];

    // 记录字符显示位置
    unsigned int xpos;
    unsigned int ypos;

    unsigned int fg_color;
    unsigned int bg_color;

    // 记录对应的显存起始位置
    unsigned long base_addr;
} tty_t;

void init_ttys();

void tty_write(tty_t *tty, const char *buf, size_t size);
void tty_write_at(tty_t *tty, int xpos, int ypos, const char *buf, size_t size);

void tty_set_cursor(tty_t *tty);
void tty_switch(tty_t *tty);

extern tty_t *current_tty;
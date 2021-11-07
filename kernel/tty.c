/*
 * ------------------------------------------------------------------------
 *   File Name: tty.c
 *      Author: Zhao Yanbai
 *              2021-11-07 17:17:18 Sunday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <assert.h>
#include <io.h>
#include <irq.h>
#include <page.h>
#include <string.h>
#include <tty.h>

#define BLACK 0b000
#define WHITE 0b111
#define RED 0b100
#define GREEN 0b010
#define BLUE 0b001

// 从0xB8000处开始有32KB显存可利用
// 而一屏所需要的显存为 80*25*2 = 4000 约为4K
// 所以大致可以分出8个tty
// 每个的起始地址以0x1000对齐
#define VADDR ((unsigned long)pa2va(0xB8000))
#define TTY_VRAM_SIZE (0x1000)

#define MAX_X 80
#define MAX_Y 25
#define CHARS_PER_LINE (MAX_X)
#define BYTES_PER_LINE (CHARS_PER_LINE * 2)
#define TAB_SPACE 4

tty_t default_tty;
tty_t monitor_tty;

void tty_clear(tty_t *tty) {
    char *dst = (char *)tty->base_addr;
    for (int src = 0; src < (MAX_Y * BYTES_PER_LINE); src += 2) {
        *dst++ = ' ';
        *dst++ = (tty->bg_color << 4) | tty->fg_color;
    }
}

void init_tty(tty_t *tty, const char *name, unsigned long base) {
    assert(0 != tty);

    memset(tty, 0, sizeof(tty_t));

    strlcpy(tty->name, name, sizeof(tty->name));

    tty->fg_color = 0x8 | GREEN;  // 高亮
    tty->bg_color = 0x0 | BLACK;  // 不闪

    tty->base_addr = base;
}

void init_ttys() {
    init_tty(&default_tty, "tty.default", VADDR + 0 * TTY_VRAM_SIZE);
    init_tty(&monitor_tty, "tty.monitor", VADDR + 1 * TTY_VRAM_SIZE);

    monitor_tty.fg_color = WHITE;
    monitor_tty.bg_color = BLUE;
    tty_clear(&monitor_tty);

    current_tty = &default_tty;
}

void tty_do_scroll_up(tty_t *tty) {
    // 不需要上卷
    if (tty->ypos < MAX_Y) {
        return;
    }

    //
    char *dst = (char *)tty->base_addr;
    for (int src = BYTES_PER_LINE; src < (MAX_Y * BYTES_PER_LINE); src++) {
        *dst++ = *(char *)(tty->base_addr + src);
    }

    // 清空最后一行
    dst = (char *)(tty->base_addr + ((MAX_Y - 1) * BYTES_PER_LINE));
    for (int i = 0; i < BYTES_PER_LINE; i += 2) {
        *dst++ = ' ';
        *dst++ = (tty->bg_color << 4) | tty->fg_color;
    }

    tty->ypos = MAX_Y - 1;
}

void tty_putc(tty_t *tty, char c) {
    bool display = false;
    switch (c) {
    case '\r':
        tty->xpos = 0;
    case '\n':
        tty->xpos = 0;
        tty->ypos += 1;
        break;
    case '\t':
        tty->xpos += TAB_SPACE;
        tty->xpos &= ~(TAB_SPACE - 1);
        tty->ypos += tty->xpos / CHARS_PER_LINE;
        tty->xpos %= CHARS_PER_LINE;
        break;
    default:
        tty->ypos += tty->xpos / CHARS_PER_LINE;
        tty->xpos %= CHARS_PER_LINE;
        display = true;
        break;
    }

    tty_do_scroll_up(tty);

    // 显示
    if (display) {
        unsigned int pos = tty->ypos * BYTES_PER_LINE + tty->xpos * 2;
        char *va = (char *)(tty->base_addr + pos);
        va[0] = c;
        va[1] = (tty->bg_color << 4) | tty->fg_color;

        tty->xpos++;
    }

    tty_set_cursor(tty);
}

void tty_write(tty_t *tty, const char *buf, size_t size) {
    assert(0 != tty);
    if (0 == buf) {
        return;
    }

    for (size_t i = 0; i < size; i++) {
        tty_putc(tty, buf[i]);
    }
}

void tty_write_at(tty_t *tty, int xpos, int ypos, const char *buf, size_t size) {
    assert(0 != tty);
    assert(xpos < BYTES_PER_LINE);
    assert(ypos < MAX_Y);
    tty->xpos = xpos;
    tty->ypos = ypos;
    tty_write(tty, buf, size);
}

#define VGA_CRTC_ADDR 0x3D4
#define VGA_CRTC_DATA 0x3D5
#define VGA_CRTC_START_ADDR_H 0xC
#define VGA_CRTC_START_ADDR_L 0xD
#define VGA_CRTC_CURSOR_H 0xE
#define VGA_CRTC_CURSOR_L 0xF

void tty_set_cursor(tty_t *tty) {
    if (tty != current_tty) {
        return;
    }
    unsigned int offset = tty->ypos * MAX_Y + tty->xpos;
    offset += VADDR;

    unsigned long flags;
    irq_save(flags);
    outb(VGA_CRTC_CURSOR_H, VGA_CRTC_ADDR);
    outb((offset >> 8) & 0xFF, VGA_CRTC_DATA);
    outb(VGA_CRTC_CURSOR_L, VGA_CRTC_ADDR);
    outb(offset & 0xFF, VGA_CRTC_DATA);
    irq_restore(flags);
}

void tty_switch(tty_t *tty) {
    if (0 == tty) {
        return;
    }

    unsigned int offset = (tty->base_addr - VADDR) / 2;

    outb(VGA_CRTC_START_ADDR_H, VGA_CRTC_ADDR);
    outb((offset >> 8) & 0xFF, VGA_CRTC_DATA);
    outb(VGA_CRTC_START_ADDR_L, VGA_CRTC_ADDR);
    outb((offset)&0xFF, VGA_CRTC_DATA);

    current_tty = tty;
}

tty_t *current_tty;
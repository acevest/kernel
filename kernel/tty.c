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

// 从0xB8000处开始有32KB显存可利用
// 而一屏所需要的显存为 80*25*2 = 4000 约为4K
// 所以大致可以分出8个tty
// 每个的起始地址以0x1000对齐
const uint32_t PHY_VADDR = 0xB8000;
const uint32_t VADDR = (uint32_t)pa2va(PHY_VADDR);
#define TTY_VRAM_SIZE (0x1000)

#define MAX_NR_TTYS 8
tty_t ttys[NR_TTYS];

#define MAX_X 80
#define MAX_Y 25
#define CHARS_PER_LINE (MAX_X)
#define BYTES_PER_LINE (CHARS_PER_LINE * 2)
#define TAB_SPACE 4

tty_t *const default_tty = ttys + 0;
tty_t *const monitor_tty = ttys + 1;
tty_t *const debug_tty = ttys + 2;

void tty_clear(tty_t *tty) {
    char *dst = (char *)tty->base_addr;
    for (int src = 0; src < (MAX_Y * BYTES_PER_LINE); src += 2) {
        *dst++ = 0;
        *dst++ = (tty->bg_color << 4) | tty->fg_color;
    }
}

// 因为光标要指向下一个待输出的位置
// 在这个位置的光标的颜色与该位置已经设置的颜色一样
// 一般输出一个字符，则光标位置在这个字符之后
// 而如果输出字符颜色与光标所处位置已经设置的颜色不一致的话
// 就需要将输出的这个字符的后一个位置的颜色设置成与之一致
// 这样光标颜色才与输出字符一致
void __tty_set_next_pos_color(tty_t *tty, char color) {
    unsigned int xpos = tty->xpos;
    unsigned int ypos = tty->ypos;

    xpos += 1;  // 指向下一个位置

    ypos += xpos / CHARS_PER_LINE;
    xpos %= CHARS_PER_LINE;

    if (ypos < MAX_Y) {
        char *dst = (char *)(tty->base_addr + ypos * BYTES_PER_LINE + 2 * xpos);
        dst[0] = 0;
        dst[1] = color;
    }
}

void init_tty(tty_t *tty, const char *name, unsigned long base) {
    assert(0 != tty);
    assert(NR_TTYS >= 1);
    assert(NR_TTYS <= MAX_NR_TTYS);

    strlcpy(tty->name, name, sizeof(tty->name));

    tty->fg_color = TTY_FG_HIGHLIGHT | TTY_GREEN;  // 高亮
    tty->bg_color = TTY_BLACK;                     // 不闪

    tty->max_x = MAX_X;
    tty->max_y = MAX_Y;

    tty->base_addr = base;

    for (int i = 0; i < TTY_VRAM_SIZE; i += 2) {
        uint8_t *p = (uint8_t *)base;
        p[i + 0] = ' ';
        p[i + 1] = (tty->bg_color << 4) | tty->fg_color;
    }
}

void init_ttys() {
    assert(irq_disabled());

    for (int i = 0; i < NR_TTYS; i++) {
        tty_t *tty = ttys + i;
        char name[sizeof(tty->name)];
        sprintf(name, "tty.%u", i);
        init_tty(tty, name, VADDR + i * TTY_VRAM_SIZE);

        if (i != TTY_WHITE) {
            tty->fg_color = TTY_FG_HIGHLIGHT | TTY_WHITE;
            tty->bg_color = i;
        } else {
            tty->fg_color = TTY_FG_HIGHLIGHT | TTY_BLACK;
            tty->bg_color = TTY_WHITE;
        }
    }

    default_tty->fg_color = TTY_FG_HIGHLIGHT | TTY_GREEN;
    default_tty->bg_color = TTY_BLACK;

    monitor_tty->fg_color = TTY_FG_HIGHLIGHT | TTY_WHITE;
    monitor_tty->bg_color = TTY_BLUE;

    debug_tty->fg_color = TTY_FG_HIGHLIGHT | TTY_WHITE;
    debug_tty->bg_color = TTY_BLACK;  // TTY_CYAN;

    for (int i = 0; i < NR_TTYS; i++) {
        tty_t *tty = ttys + i;
        tty_clear(tty);
    }
    current_tty = default_tty;
}

void tty_do_scroll_up(tty_t *tty) {
    // 没越过最后一行不需要上卷
    if (tty->ypos < MAX_Y - 1) {
        return;
    }

    // 达到最后一行，没到最后一个字符也不用上卷
    if (tty->ypos == (MAX_Y - 1) && tty->xpos < MAX_X) {
        return;
    }

    // 如果是default_tty则保留用来显示内核版本及编译时间信息
    const int keep = tty != default_tty ? 0 : BYTES_PER_LINE;

    char *dst = (char *)tty->base_addr + keep;
    for (int src = BYTES_PER_LINE + keep; src < (MAX_Y * BYTES_PER_LINE); src++) {
        *dst++ = *(char *)(tty->base_addr + src);
    }

    // 清空最后一行
    dst = (char *)(tty->base_addr + ((MAX_Y - 1) * BYTES_PER_LINE));
    for (int i = 0; i < BYTES_PER_LINE; i += 2) {
        *dst++ = 0;
        *dst++ = (tty->bg_color << 4) | tty->fg_color;
    }

    tty->xpos = 0;
    tty->ypos = MAX_Y - 1;
}

void tty_color_putc(tty_t *tty, char c, unsigned int fg_color, unsigned bg_color) {
    bool display = false;
    bool move_to_next_pos = true;
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
        break;
    case '\b':
        if (tty->xpos == 0) {
            if (tty->ypos > 0) {
                tty->xpos = CHARS_PER_LINE - 1;
                tty->ypos -= 1;
            } else {
                tty->ypos = 0;
            }
        } else if (tty->xpos > 0) {
            tty->xpos -= 1;
        } else {
            tty->xpos = 0;
        }
        c = 0;
        display = true;
        move_to_next_pos = false;
        break;
    default:
        display = true;
        break;
    }

    tty->ypos += tty->xpos / CHARS_PER_LINE;
    tty->xpos %= CHARS_PER_LINE;

    // 显示
    if (display) {
        unsigned int pos = tty->ypos * BYTES_PER_LINE + tty->xpos * 2;
        char *va = (char *)(tty->base_addr + pos);
        va[0] = c;
        va[1] = (bg_color << 4) | fg_color;

        __tty_set_next_pos_color(tty, va[1]);

        if (move_to_next_pos) {
            tty->xpos++;
        }
    }

    tty_do_scroll_up(tty);

    tty_set_cursor(tty);
}

void tty_putc(tty_t *tty, char c) { tty_color_putc(tty, c, tty->fg_color, tty->bg_color); }

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
    unsigned int offset = tty->ypos * MAX_X + tty->xpos;

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

    unsigned long flags;
    irq_save(flags);
    outb(VGA_CRTC_START_ADDR_H, VGA_CRTC_ADDR);
    outb((offset >> 8) & 0xFF, VGA_CRTC_DATA);
    outb(VGA_CRTC_START_ADDR_L, VGA_CRTC_ADDR);
    outb((offset) & 0xFF, VGA_CRTC_DATA);
    irq_restore(flags);

    current_tty = tty;

    tty_set_cursor(current_tty);
}

void tty_switch_to_next() {
    tty_t *tty = ttys + ((current_tty - ttys + 1) % NR_TTYS);
    tty_switch(tty);
}

tty_t *current_tty;

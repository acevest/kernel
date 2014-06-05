/*
 *--------------------------------------------------------------------------
 *   File Name: vga.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sat Jul 18 23:01:18 2009
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */

#include <io.h>

typedef struct {
    u8_t    c;
    u8_t    f;
} __attribute__((packed)) vga_char_t;

#define VIDEO_ADDR              0xC00B8000

#define VGA_CRTC_ADDR           0x3D4
#define VGA_CRTC_DATA           0x3D5
#define VGA_CRTC_START_ADDR_H   0xC
#define VGA_CRTC_START_ADDR_L   0xD
#define VGA_CRTC_CURSOR_H       0xE
#define VGA_CRTC_CURSOR_L       0xF

#define LINES_PER_SCREEN        25
#define CHARS_PER_LINE          80
#define BYTES_PER_LINE          (sizeof(vga_char_t)*CHARS_PER_LINE)

#define TAB_ALIGN               4
#define TAB_MASK                (TAB_ALIGN-1)

static unsigned int g_offset = 0;

static void set_offset(unsigned int x, unsigned int y)
{
    g_offset = (y * CHARS_PER_LINE) + x;
}

static unsigned int xpos()
{
    return (g_offset % CHARS_PER_LINE);
}

static unsigned int ypos()
{
    return (g_offset / CHARS_PER_LINE);
}

vga_char_t vga_char(unsigned char c, unsigned char f)
{
    vga_char_t x;
    x.c = c;
    x.f = f;

    return x;
}

void vga_set_cursor_pos()
{
    outb(VGA_CRTC_CURSOR_H,     VGA_CRTC_ADDR);
    outb((g_offset>>8) & 0xFF,  VGA_CRTC_DATA);
    outb(VGA_CRTC_CURSOR_L,     VGA_CRTC_ADDR);
    outb(g_offset & 0xFF,       VGA_CRTC_DATA);
}

void vga_clear(unsigned int b, unsigned int e)
{
    vga_char_t *base = (vga_char_t *) VIDEO_ADDR;

    base += b;

    memset((void *)base, 0, (e-b)*sizeof(vga_char_t));
}

void vga_scroll_up()
{
    int delta = ypos() + 1 - LINES_PER_SCREEN;

    if(delta <= 0)
        return;

    vga_char_t *base = (vga_char_t *) VIDEO_ADDR;
    vga_char_t *head = base + delta*CHARS_PER_LINE;
    vga_char_t *empt = base + (LINES_PER_SCREEN-delta)*CHARS_PER_LINE;

    memcpy((void *)base, (void *)head, (empt-base)*sizeof(vga_char_t));
    //memset((void *)empt, 0, delta*BYTES_PER_LINE);

    vga_clear((LINES_PER_SCREEN-delta)*CHARS_PER_LINE, LINES_PER_SCREEN*CHARS_PER_LINE);

    set_offset(xpos(), ypos() - delta);
}



void vga_putc(const unsigned char c, const unsigned char color)
{
    vga_char_t * const pv = (vga_char_t * const) VIDEO_ADDR;

    bool need_clear = true;
    unsigned int old_offset = g_offset;
    
    switch(c)
    {
    case '\r':
        set_offset(0, ypos());
        break;
    case '\n':
        set_offset(0, ypos() + 1);
        break;
    case '\t':
        set_offset((xpos() + 1 + TAB_MASK) & ~TAB_MASK, ypos());
        break;
    default:
        need_clear = false;
        pv[g_offset] = vga_char(c, color);
        set_offset(xpos()+1, ypos());
        break;
    }

    if(need_clear)
    {
        vga_clear(old_offset, g_offset);
    }

    vga_scroll_up();

    vga_set_cursor_pos();
}

void vga_puts(const char *buf, unsigned char color)
{
    char *p = (char *) buf;

    while(*p)
    {
        vga_putc(*p, color);
        p++;
    }
}

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
#include <irq.h>

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
#define MAX_LINES_PER_SCREEN    32

#define TAB_ALIGN               4
#define TAB_MASK                (TAB_ALIGN-1)

typedef struct {
    unsigned int id;
    vga_char_t *base;
    unsigned int offset;
} vga_screen_t;

#define VGA_MAX_SCREEN_CNT      4
unsigned int vga_screen_cnt() {return VGA_MAX_SCREEN_CNT;}
vga_screen_t vga_screen[VGA_MAX_SCREEN_CNT] = {
    {
        0,
        (vga_char_t *)VIDEO_ADDR,
        0
    },
};

static vga_screen_t *vga_current_screen = vga_screen + 0;

static void set_offset(vga_screen_t *s, unsigned int x, unsigned int y)
{
    s->offset = (y * CHARS_PER_LINE) + x;
}

static unsigned int xpos(vga_screen_t *s)
{
    return (s->offset % CHARS_PER_LINE);
}

static unsigned int ypos(vga_screen_t *s)
{
    return (s->offset / CHARS_PER_LINE);
}

vga_char_t vga_char(unsigned char c, unsigned char f)
{
    vga_char_t x;
    x.c = c;
    x.f = f;

    return x;
}

void vga_set_cursor_pos(vga_screen_t *s)
{
    unsigned int base = s->id*MAX_LINES_PER_SCREEN*CHARS_PER_LINE;
    unsigned int offset = base + s->offset;
    base = s->id*MAX_LINES_PER_SCREEN*CHARS_PER_LINE;
    offset = base + s->offset;

    unsigned long flags;
    irq_save(flags);
    outb(VGA_CRTC_CURSOR_H,     VGA_CRTC_ADDR);
    outb((offset>>8) & 0xFF,    VGA_CRTC_DATA);
    outb(VGA_CRTC_CURSOR_L,     VGA_CRTC_ADDR);
    outb(offset & 0xFF,         VGA_CRTC_DATA);
    irq_restore(flags);
}

void vga_clear(vga_screen_t *s, unsigned int b, unsigned int e)
{
    if(e <= b)
        return ;

    vga_char_t *base = s->base;

    base += b;

    memset((void *)base, 0, (e-b)*sizeof(vga_char_t));
}

void vga_scroll_up(vga_screen_t *s)
{
    int delta = ypos(s) + 1 - LINES_PER_SCREEN;

    if(delta <= 0)
        return;

    vga_char_t *base = s->base;
    vga_char_t *head = base + delta*CHARS_PER_LINE;
    vga_char_t *empt = base + (LINES_PER_SCREEN-delta)*CHARS_PER_LINE;

    memcpy((void *)base, (void *)head, (empt-base)*sizeof(vga_char_t));
    //memset((void *)empt, 0, delta*BYTES_PER_LINE);

    vga_clear(s, (LINES_PER_SCREEN-delta)*CHARS_PER_LINE, LINES_PER_SCREEN*CHARS_PER_LINE);

    set_offset(s, xpos(s), ypos(s) - delta);
}



void vga_putc(vga_screen_t *s, const unsigned char c, const unsigned char color)
{
    vga_char_t *pv = s->base;

    bool need_clear = true;
    unsigned int old_offset = s->offset;
    
    switch(c)
    {
    case '\r':
        set_offset(s, 0, ypos(s));
        break;
    case '\n':
        set_offset(s, 0, ypos(s) + 1);
        break;
    case '\t':
        set_offset(s, (xpos(s) + 1 + TAB_MASK) & ~TAB_MASK, ypos(s));
        break;
    default:
        need_clear = false;
        pv[s->offset] = vga_char(c, color);
        set_offset(s, xpos(s)+1, ypos(s));
        break;
    }

    if(need_clear)
    {
        vga_clear(s, old_offset, s->offset);
    }

    vga_scroll_up(s);

    if(vga_current_screen == s)
        vga_set_cursor_pos(s);
}


void vga_puts(unsigned int nr, const char *buf, unsigned char color)
{
    if(nr >= VGA_MAX_SCREEN_CNT)
        return ;

    char *p = (char *) buf;
    vga_screen_t *s = vga_screen + nr;

    while(*p)
    {
        vga_putc(s, *p, color);
        p++;
    }
}

void __vga_switch(unsigned int offset)
{
    outb(VGA_CRTC_START_ADDR_H, VGA_CRTC_ADDR);
    outb((offset>>8)&0xFF,      VGA_CRTC_DATA);
    outb(VGA_CRTC_START_ADDR_L, VGA_CRTC_ADDR);
    outb((offset)&0xFF,         VGA_CRTC_DATA);
}

int bvga = 0;
void vga_init()
{
    unsigned int i;
    for(i=1; i<VGA_MAX_SCREEN_CNT; ++i)
    {
        memset(vga_screen + i, 0, sizeof(vga_screen_t));
        vga_screen[i].id    = i;
        vga_screen[i].base  = (vga_char_t *) (VIDEO_ADDR + i*MAX_LINES_PER_SCREEN*BYTES_PER_LINE);
        memset(vga_screen[i].base, 0, MAX_LINES_PER_SCREEN*BYTES_PER_LINE);
    }
    bvga = 1;
}

void vga_switch(unsigned int nr)
{
    if(nr >= VGA_MAX_SCREEN_CNT)
        return ;

    vga_screen_t *s = vga_screen + nr;

    vga_current_screen = s;

    unsigned int offset = 0 + (s->base - (vga_char_t*)VIDEO_ADDR);

    __vga_switch(offset);
}

#define VIDEO_DBG_LINE ((VGA_MAX_SCREEN_CNT)*(MAX_LINES_PER_SCREEN))

void vga_dbg_toggle()
{
    static bool dbg = true;
    unsigned int offset = 0;
    if(dbg)
    {
        offset += VIDEO_DBG_LINE*CHARS_PER_LINE;
    }

    dbg = !dbg;

    __vga_switch(offset);
}


void vga_dbg_puts(unsigned int line, const char *buf, unsigned char color)
{
    int i;
    char *p = (char *) buf;
    vga_char_t * const pv = (vga_char_t * const) (VIDEO_ADDR + (VIDEO_DBG_LINE + line) * BYTES_PER_LINE);

    for(i=0; i<CHARS_PER_LINE; ++i)
        pv[i] = vga_char(0, color);

    for(i=0; *p; ++i, ++p)
    {
        color = 0x1F;
        pv[i] = vga_char(*p, color);
    }
}

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

#define VGA_CRTC_ADDR    0x3D4
#define VGA_CRTC_DATA    0x3D5
#define VGA_CRTC_START_ADDR_H    0xC
#define VGA_CRTC_START_ADDR_L    0xD
#define VGA_CRTC_CURSOR_H    0xE
#define VGA_CRTC_CURSOR_L    0xF


#define LINES_PER_SCREEN    24    //25
#define CHARS_PER_LINE        80
#define BYTES_PER_LINE        (CHARS_PER_LINE<<1)
#define MAX_LINES        (LINES_PER_SCREEN<<1)
#define VIDEO_ADDR        0xC00B8000

static unsigned int xpos, ypos;

void    setCursorPos(unsigned int x, unsigned int y);
void     printString(const char *buf, unsigned char color);
void    printChar(const unsigned char c, const unsigned char color);
void    ScrollUp();

void    setCursorPos(unsigned int x, unsigned int y)
{
    unsigned short    offset = x +  y * CHARS_PER_LINE;
    outb(VGA_CRTC_CURSOR_H,    VGA_CRTC_ADDR);
    outb((offset>>8) & 0xFF,VGA_CRTC_DATA);
    outb(VGA_CRTC_CURSOR_L,    VGA_CRTC_ADDR);
    outb(offset & 0xFF,    VGA_CRTC_DATA);
}

void    printChar(const unsigned char c, const unsigned char color)
{
    unsigned short * const pv = (unsigned short * const) VIDEO_ADDR;

    switch(c)
    {
    case '\r':
        xpos = 0;
        break;
    case '\n':
        xpos = 0; ypos++;
        break;
    case '\t':
        xpos &= (~0x07);
        xpos += 8;
        if(xpos >= CHARS_PER_LINE)
        {
            xpos = 0;
            ypos ++;
        }
        break;
    case '\b':
        if(xpos > 0)
            xpos--;
        else if(ypos > 0)
        {
            xpos = CHARS_PER_LINE - 1;
            ypos --;
        }
        *(pv + xpos + ypos*CHARS_PER_LINE) = ' ' | (color << 8);
        break;
    default:
        *(pv + xpos + ypos*CHARS_PER_LINE) = c | (color << 8);
        xpos ++;
        if(xpos == CHARS_PER_LINE)
        {
            xpos = 0;
            ypos ++;
        }
        break;
    }

    ScrollUp();

    setCursorPos(xpos, ypos);
}

void    ScrollUp()
{
    static unsigned int last_ypos;
    unsigned short topline;
    if((ypos >= LINES_PER_SCREEN) && (ypos > last_ypos))
    {
        topline = ypos - LINES_PER_SCREEN;

        if(topline == MAX_LINES)
        {

            memcpy(    VIDEO_ADDR,
                VIDEO_ADDR+MAX_LINES*BYTES_PER_LINE,
                LINES_PER_SCREEN*BYTES_PER_LINE);

            unsigned char *p;
            const unsigned char *pend;
            p=(unsigned char *)
            VIDEO_ADDR+LINES_PER_SCREEN*BYTES_PER_LINE;
            while(p <=(unsigned char *) VIDEO_ADDR 
            + (LINES_PER_SCREEN+MAX_LINES)*BYTES_PER_LINE)
            {
                *p++ = ' ';
                *p++ = 0x0F;
            }
            ypos -= topline;
            topline = 0;
        }
        outb(VGA_CRTC_START_ADDR_H,VGA_CRTC_ADDR);
        outb(((topline*CHARS_PER_LINE)>>8)&0xFF,VGA_CRTC_DATA);
        outb(VGA_CRTC_START_ADDR_L,VGA_CRTC_ADDR);
        outb((topline*CHARS_PER_LINE)&0xFF,VGA_CRTC_DATA);
    }
    last_ypos = ypos;
}

void     printString(const char *buf, unsigned char color)
{
    char *p = (char *) buf;

    while(*p)
    {
        printChar(*p, color);
        p++;
    }
}

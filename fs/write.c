/*
 *--------------------------------------------------------------------------
 *   File Name: write.c
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0
 * Create Date: Sun Mar  8 11:05:12 2009
 * Last Update: Sun Mar  8 11:05:12 2009
 * 
 *--------------------------------------------------------------------------
 */


extern void     vga_puts(const char *buf, unsigned char color);
int sysc_write(int fd, const char *buf, unsigned long size)
{
    if(size < 0) return -1;

    switch(fd)
    {
    case 0:
        vga_puts(buf, 0xF);
        break;
    default:
        return -1;
    }

    return size;
}

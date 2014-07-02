/*
 *--------------------------------------------------------------------------
 *   File Name: printk.c
 * 
 * Description: none
 * 
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:    1.0 Wed Jul 30 14:25:23 2008
 *     Version: 1.1 Tue Feb 10 22:40:25 2009
 *     Version:    2.0 Sun Mar 08 10:52:51 2009
 *     Version:    3.0 Sat Jul 18 23:06:27 2009
 * 
 *--------------------------------------------------------------------------
 */

extern void vga_puts(unsigned int nr, const char *buf, unsigned char color);
extern void vga_dbg_puts(unsigned long line, const char *buf, unsigned char color);

unsigned int printk_screen_nr = 0;

extern unsigned int vga_screen_cnt();
void switch_printk_screen()
{
    printk_screen_nr++;
    printk_screen_nr %= vga_screen_cnt();
}

char pkbuf[1024];
int printk(const char *fmtstr, ...)
{
    char *args = (char*)(((char*)&fmtstr)+4);
    vsprintf(pkbuf, fmtstr, args);
    vga_puts(printk_screen_nr, pkbuf, 0x2);
    return 0;
}

char pdbuf[1024];
int printd(unsigned int line, const char *fmtstr, ...)
{
    char *args = (char*)(((char*)&fmtstr)+4);
    vsprintf(pdbuf, fmtstr, args);
    vga_dbg_puts(line, pdbuf, 0x7);
    return 0;
}

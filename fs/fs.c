/*
 *--------------------------------------------------------------------------
 *   File Name: fs.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Fri Feb 12 20:48:50 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */
#include <fs.h>
#include <io.h>
#include <printk.h>
#include <system.h>

extern chrdev_t cnsl_chrdev;

chrdev_t *chrdev[CHRDEV_SIZE] = {
    &cnsl_chrdev};

void ext2_setup_fs();
unsigned int ext2_search_inpath(const char *path);

void setup_fs()
{
    ext2_setup_fs();
}

unsigned int namei(const char *path)
{
    return ext2_search_inpath(path);
}

/*
 *--------------------------------------------------------------------------
 *   File Name: kmalloc_old.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sat Jan 30 12:15:51 2010
 * 
 * Description: 现在的版本实现得非常简单，简陋。不能分配大于32*4K的内存
 *         另外小于32*4K的都按32*4K分配
 *         以后再来重写这里吧
 *--------------------------------------------------------------------------
 */
#include <page.h>
#include <types.h>
#include <assert.h>
#include <system.h>

static int get_order(size_t size)
{
    int i;
    return i;
}
void    *kmalloc_old(size_t size)
{
    void    *p;
    return p;
}


void    kfree_old(void *p)
{

}

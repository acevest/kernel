/*
 * ------------------------------------------------------------------------
 *   File Name: blk_rw.c
 *      Author: Zhao Yanbai
 *              Tue Jul  8 22:21:37 2014
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <fs.h>
#include <ide.h>
#include <system.h>

// only support read
void blk_rw(dev_t dev, u64_t offset, u32_t size, char *buf) {
    assert(DEV_MAJOR(dev) == DEV_MAJOR_HDA);
    assert(offset % SECT_SIZE == 0);
    assert(size % SECT_SIZE == 0);

    const part_t *p = ide_get_part(dev);

    u64_t lba = p->lba_start;
    lba += offset / SECT_SIZE;

    assert(lba < p->lba_end);

    u32_t scnt = size / SECT_SIZE;

    printd("%s lba %u scnt %u\n", __func__, (u32_t)lba, scnt);

    ide_do_read(lba, scnt, buf);
}

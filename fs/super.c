/*
 * ------------------------------------------------------------------------
 *   File Name: super.c
 *      Author: Zhao Yanbai
 *              2024-08-30 21:38:05 Friday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include "errno.h"
#include "fs.h"
#include "irq.h"
#include "list.h"
#include "system.h"

LIST_HEAD(g_superblocks);

static superblock_t* alloc_super(fs_type_t* type) {
    superblock_t* s;
    s = (superblock_t*)kzalloc(sizeof(superblock_t), 0);
    if (0 == s) {
        panic("alloc superblock for %s", type->name);
        return s;
    }

    static sb_operations_t default_sb_ops;

    s->sb_ops = &default_sb_ops;
    INIT_LIST_HEAD(&s->sb_list);
    INIT_LIST_HEAD(&s->sb_instance);

    return s;
}

static uint32_t __minor = 0;
int set_anonymous_super(superblock_t* s, void* data) {
    s->sb_dev = MAKE_DEV(0, ++__minor);
    return 0;
}

int read_super_for_nodev(fs_type_t* type, int flags, void* data, fill_super_cb_t fill_super, vfsmount_t* mnt) {
    int err = 0;
    superblock_t* s = 0;

    // 分配superblock
    err = sget(type, NULL, set_anonymous_super, NULL, &s);

    assert(s != NULL);

    //
    s->sb_flags = flags;

    err = fill_super(s, data);
    if (0 != err) {
        panic("err: %d", err);
    }

    mnt->mnt_sb = s;
    mnt->mnt_root = dentry_get(s->sb_root);

    return err;
}

int sget(fs_type_t* type,                    //
         int (*test)(superblock_t*, void*),  //
         int (*set)(superblock_t*, void*),   //
         void* data,                         //
         superblock_t** s                    //
) {
    int err = 0;

    *s = 0;

    if (0 != test) {
        panic("not implemented");
    }

    // TOOD REMOVE
    assert(0 == *s);

    *s = alloc_super(type);
    if (0 == *s) {
        return ENOMEM;
    }

    assert(0 != set);

    err = set(*s, data);
    assert(0 == err);

    uint32_t eflags;
    irq_save(eflags);
    list_add_tail(&(*s)->sb_list, &g_superblocks);
    irq_restore(eflags);

    return err;
}

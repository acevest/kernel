/*
 * ------------------------------------------------------------------------
 *   File Name: mount.c
 *      Author: Zhao Yanbai
 *              2024-08-30 21:40:57 Friday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include "mount.h"

#include "irq.h"
#include "mm.h"
#include "string.h"
#include "system.h"
#include "task.h"

kmem_cache_t *vfsmount_kmem_cache = 0;

// 通过挂载点目录的 path_t 也就是 {mount, dentry}计算hash
// 就可以得到所有挂载在该目录上的挂载描述符 vfsmount
vfsmount_t **vfsmount_hash_table = 0;
int vfsmount_hash_table_size = 0;

vfsmount_t *alloc_vfsmount(const char *name) {
    vfsmount_t *mnt = 0;

    mnt = (vfsmount_t *)kmem_cache_zalloc(vfsmount_kmem_cache, 0);

    if (0 == mnt) {
        panic("no mem alloc for vfsmount: %s", name);
        return 0;
    }

    INIT_LIST_HEAD(&mnt->mnt_list);
    strncpy(mnt->mnt_devname, name, MAX_VFSMNT_NAME_LEN);

    assert(mnt->hash_next == 0);

    return mnt;
}

vfsmount_t *vfs_kernel_mount(fs_type_t *type, int flags, const char *name, void *data) {
    int ret = 0;
    vfsmount_t *mnt = 0;

    assert(0 != type);

    mnt = alloc_vfsmount(name);

    ret = type->read_super(type, flags, name, data, mnt);

    assert(mnt->mnt_sb != 0);
    assert(mnt->mnt_root != 0);

    mnt->mnt_point = mnt->mnt_root;
    mnt->mnt_parent = mnt;

    // if (type->flags & FS_TYPE_NODEV) {
    // }

    return mnt;
}

unsigned long vfsmount_table_hash(vfsmount_t *mnt, dentry_t *dentry) {
    unsigned long h = (unsigned long)mnt / 5;
    h += 1;
    h += (unsigned long)dentry / 5;
    h = h + (h >> 9);
    return h & (vfsmount_hash_table_size - 1);
}

void add_vfsmount_to_hash_table(vfsmount_t *mnt) {
    unsigned long hash = vfsmount_table_hash(mnt->mnt_parent, mnt->mnt_point);

    uint32_t eflags;
    irq_save(eflags);

    vfsmount_t **p = vfsmount_hash_table + hash;

    mnt->hash_next = *p;

    *p = mnt;

    irq_restore(eflags);
}

void init_mount() {
    vfsmount_kmem_cache = kmem_cache_create("vfsmount", sizeof(vfsmount_t), 4);
    if (0 == vfsmount_kmem_cache) {
        panic("create vfsmount kmem cache failed");
    }

    vfsmount_hash_table = (vfsmount_t **)page2va(alloc_one_page(0));
    memset(vfsmount_hash_table, 0, PAGE_SIZE);

    vfsmount_hash_table_size = PAGE_SIZE / sizeof(vfsmount_t *);

    assert(vfsmount_hash_table_size != 0);

    // vfsmount_hash_table_size 应该是2的n次方的数
    int bit1_cnt = 0;
    for (int i = 0; i < sizeof(vfsmount_hash_table_size) * 8; i++) {
        if (vfsmount_hash_table_size & (1 << i)) {
            bit1_cnt++;
        }
    }
    assert(bit1_cnt == 1);
}

void mount_root() {
    fs_type_t *type = vfs_find_filesystem("ramfs");
    assert(type != NULL);

    vfsmount_t *mnt = vfs_kernel_mount(type, 0, "ramfs", NULL);
    assert(mnt != NULL);

    assert(mnt->mnt_root != NULL);

    current->root.mnt = mnt;
    current->root.dentry = mnt->mnt_root;
    current->pwd.mnt = mnt;
    current->pwd.dentry = mnt->mnt_root;
}

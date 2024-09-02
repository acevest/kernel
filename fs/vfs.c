/*
 * ------------------------------------------------------------------------
 *   File Name: vfs.c
 *      Author: Zhao Yanbai
 *              2021-11-27 11:59:30 Saturday CST
 * Description: none
 * ------------------------------------------------------------------------
 */

#include <errno.h>
#include <string.h>
#include <system.h>
#include <vfs.h>

// 要访问一个文件就必需先访问一个目录，才能根据文件名从目录中找到该文件的目录项，进而找到其inode
// 但是目录本身也是文件，它本身的目录项dentry又在另一个目录项中
// 这个递归问题的出口在于，必然有一个目录，它的目录项不在其它目录中，而在一个固定的位置上
// 或者可通过固定的算法找到，且从该目录出发就可以找到系统中的所有文件
// 这个目录就是根目录"/"，或者说是根设备上的根目录。
// 每个文件系统都有一个根目录和一个超级块，根目录的位置及文件系统中的其它参数就记录在超级块中
// 超级块在设备上的逻辑位置对每个文件系统来说都是固定的
// 系统在初始化的时候要将一个存储设备作为整个系统的根设备，它的根目录就成为整个文件系统的总根目录，即"/"
// 更确切地说，就是把根设备的根目录安装在文件系统的的总根"/"节点上。
// 有了根设备以后，还可以把其它存储设备也安装到文件系统的空闲目录节点上。
// 所谓“安装“就是从一个存储设备上读入超级块，在内存中建立起一个superblock结构。进而将此设备上的根目录
// 与文件系统中已经存在的一个空白目录挂上钩。
// 系统初始化时整个文件系统只有一个空白目录"/"，所以根设备的根目录就安装到这个节点上。
dentry_t *root_entry = 0;

fs_type_t file_systems = {"filesystems", 0, 0};

int vfs_register_filesystem(fs_type_t *fs) {
    int ret = 0;

    assert(fs != NULL);
    if (fs->next != NULL) {
        return -EBUSY;
    }

    INIT_LIST_HEAD(&fs->sbs);

    fs_type_t *add = &file_systems;

    // TODO: 加锁、解锁保护

    for (fs_type_t *fst = &file_systems; fst != 0; fst = fst->next) {
        if (strcmp(fst->name, fs->name) == 0) {
            return -EBUSY;
        }
        add = fst;
    }

    add->next = fs;
    fs->next = 0;
    return 0;
}

fs_type_t *vfs_find_filesystem(const char *name) {
    for (fs_type_t *fs = &file_systems; fs != 0; fs = fs->next) {
        if (strcmp(fs->name, name) == 0) {
            return fs;
        }
    }

    return NULL;
}

// void ramfs_init();
// void vfs_init() {
//     ramfs_init();
//     fs_type_t *fs = vfs_find_filesystem("ramfs");
//     if (NULL == fs) {
//         panic("no ramfs");
//     }

//     // superblock_t *sb = fs->read_super(NULL, NULL);
// }

/////////
vfsmount_t *vfsmnt_get(vfsmount_t *m) {
    panic("todo");
    return NULL;
}
void vfsmnt_put(vfsmount_t *m) {
    //
    panic("todo");
}

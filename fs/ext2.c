/*
 *--------------------------------------------------------------------------
 *   File Name: ext2.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sun Feb 14 15:52:01 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */

#include <fs.h>
#include <system.h>
#include <string.h>
#include <stat.h>

unsigned int ext2_start_sect;
Inode    ext2_root_inode;

extern void hd_read(Dev dev, u64 sect_nr, void *buf, u32 count);
void    ext2_read_block(int block_id, char *buf);
static void ext2_print_group_descriptor(pGroupDesc p);
static void ext2_print_inode(pInode p);
int ext2_search_file(const char *file, const pInode in, pInode out);
//char *ext2_loadfile(const char *path, size_t *file_size);

/* ext2 super block */
SuperBlock ext2_sb
    __attribute__((__aligned__(PAGE_SIZE)));
Inode    inode_table[NR_INODES]
    __attribute__((__aligned__(PAGE_SIZE))) = {{0,},};
/* 
 * ext2 group descriptor table
 * 其大小为一个EXT2_BLOCK_SIZE
 */
static pGroupDesc gdesc;

void    setup_ext2()
{
    int i;

    hd_read(ROOT_DEV, ext2_start_sect+2, EXT2_SB, sizeof(SuperBlock));
    printk("EXT2 SB:%x\n", sizeof(SuperBlock));
    printk("inodes count: %d\n", EXT2_SB->s_inodes_count);
    printk("blocks count: %d\n", EXT2_SB->s_blocks_count);
    printk("s_magic: %x\n", EXT2_SB->s_magic);
    printk("LOG BLOCK SIZE:%x\n", EXT2_SB->s_log_block_size);
    printk("Block Size:%d %x\n", EXT2_BLOCK_SIZE, EXT2_BLOCK_SIZE);


    if(EXT2_SB->s_magic != 0xEF53)
        panic("Only Support Ext2 File System...");

    /* 分配group descriptor table 的内存 */
    gdesc = (pGroupDesc) kmalloc_old(EXT2_BLOCK_SIZE);
    if(gdesc == NULL)
        panic("out of memory for ext2 group descritpor table");
    /* 初始化ext2 group descriptor table */
    ext2_read_block(EXT2_FIRST_BLOCK_ID+1, (char *)gdesc);
#if 0
    printk("-------%d %d %d",
        EXT2_BLOCK_SIZE, EXT2_INODE_SIZE, EXT2_INODES_PER_BLOCK);
    for(i=0; i<12; i++)
        ext2_print_group_descriptor(gdesc+i);
#endif

    ext2_read_inode(2, &ext2_root_inode);
#if 1
    ext2_print_inode(&ext2_root_inode);
#endif

#if 0
    Inode    boot_ino, grub_ino, grubconf_ino;
    int    boot_n, grub_n, grubconf_n;
    boot_n = ext2_search_file("boot", &ext2_root_inode, &boot_ino);
    printk("boot_n:%d size:%d\n", boot_n, boot_ino.i_size);
    grub_n = ext2_search_file("grub", &boot_ino, &grub_ino);
    printk("grub_n:%d size:%d\n", grub_n, grub_ino.i_size);
    grubconf_n = ext2_search_file("grub.conf",
                    &grub_ino, &grubconf_ino);
    printk("grubconf_n:%d size:%d\n", grubconf_n, grubconf_ino.i_size);
#endif
#if 0
    size_t filesz;
    char *buf=ext2_loadfile("/boot/grub/grub.conf", &filesz);

    for(i=0; i<filesz; i++)
        printk("%c", buf[i]);
#endif
#if 0
    unsigned char *p = (char *)kmalloc_old(EXT2_BLOCK_SIZE);
    ext2_read_block(0, p);

    for(i=0; i<512; i++)
        printk("%02x ", p[i]);
#endif
#if 0
    int dly = 0xF000000;
    while(dly--);
#endif
}

void    ext2_read_block(int block_id, char *buf)
{
    u64 sect = EXT2_SECT;
    sect += block_id*EXT2_SECT_PER_BLOCK;

    //printk("block id: %d sect:%d\n", block_id, sect);

    hd_read(ROOT_DEV, sect, buf, EXT2_BLOCK_SIZE);
}


/*
 * 写这版ext2_read_inode的时候我想起了2008.12.16在0.2.1中写的
 * 那个 GetInode. 这两个函数完成的是同样的功能.但是很明显
 * 之前的那个版本写得那是相当的滑稽幼稚.甚至还有大大的BUG.
 * 但是正是这些点滴见证着我的内核的成长.
 */
int    ext2_read_inode(unsigned int n, pInode ino)
{
    assert(1<=n && n <= EXT2_INODES_COUNT);
    if(n == EXT2_BAD_INO) return 0;

    unsigned int    gnr;    /* inode 所在的group的号 */
    unsigned int    gidx;    /* inode 所在的group的偏移 */
    unsigned int    inotbl;    /* inode table 的 block id */


    gnr    = (n-1) / EXT2_INODES_PER_GROUP;
    gidx    = (n-1) % EXT2_INODES_PER_GROUP;

    inotbl    = (gdesc+gnr)->bg_inode_table;

    // 跳过完整的BLOCK
    inotbl += (gidx/EXT2_INODES_PER_BLOCK);
    gidx   %= EXT2_INODES_PER_BLOCK;


    char *buf = kmalloc_old(EXT2_BLOCK_SIZE);
    if(buf == NULL)
        panic("faild read inode. out of memory");
    ext2_read_block(inotbl, buf);

    memcpy((void *)ino,(void*)(((pInode)buf)+gidx), EXT2_INODE_SIZE);

    kfree_old(buf);

    return n;
}
int ext2_read_file(const pInode ino, void *buf, size_t count)
{
    int i, blks;
    void *p;

    blks = (ino->i_size+EXT2_BLOCK_SIZE-1)/EXT2_BLOCK_SIZE;
    assert(blks>0);

    if(blks > EXT2_NDIR_BLOCKS)
        panic("file too large to read");

    p = kmalloc_old(blks*EXT2_BLOCK_SIZE);

    if(p == NULL)
        panic("out of memory when search inode in directory");

    for(i=0; i<blks; i++)
    {
        ext2_read_block(ino->i_block[i],
            (char *)(i*EXT2_BLOCK_SIZE + (unsigned long)p));
    }

    memcpy(buf, p, count);

    kfree_old(p);

    return count;
}
#if 0
char *load_inode_content(const pInode ino)
{
    int i, blks;
    char *buf;

    blks = (ino->i_size+EXT2_BLOCK_SIZE-1)/EXT2_BLOCK_SIZE;
    assert(blks>0);
    if(blks > EXT2_NDIR_BLOCKS)
        panic("unsupport file large than 12KB");


    buf = kmalloc_old(blks*EXT2_BLOCK_SIZE);
    if(buf == NULL)
        panic("out of memory when search inode in directory");
    for(i=0; i<blks; i++)
    {
        ext2_read_block(ino->i_block[i], buf + i*EXT2_BLOCK_SIZE);
    }

    return buf;
}
#endif

/*
 * 从in中查找file从out返回
 * int 型的函数返回值的意思是inode号
 * 失败则返回的是EXT2_BAD_INO
 */
int ext2_search_file(const char *file, const pInode in, pInode out)
{
    assert(file != NULL);
    assert(in != NULL);
    assert(S_ISDIR(in->i_mode));

    char *buf;
#if 0
    buf = load_inode_content(in);
#else
    buf = kmalloc_old(in->i_size);
    ext2_read_file(in, buf, in->i_size);
#endif

    pDirEnt    ent = (pDirEnt)buf;
    int len = strlen(file);
    int inode_n = EXT2_BAD_INO;
    while(ent < (pDirEnt)(buf + in->i_size))
    {
        if(ent->name_len == len)
        {
            if(strncmp(file, ent->name, ent->name_len) == 0)
            {
                inode_n = ent->inode;
#if 0
                printk("file: %s\n", ent->name);
#endif
                break;
            }
        }


        ent = (pDirEnt)(ent->rec_len + (unsigned long)ent);
    }
    kfree_old(buf);

    ext2_read_inode(inode_n, out);

    return inode_n;
}


static int get_filename_from_path(const char *path, char *filename)
{
    int i = 0;

    while(*path != '/' && *path != '\0')
    {
        filename[i++] = *path++;
    }
    filename[i] = 0;


    return i; // file name len
}


int ext2_get_file_inode_nr(const char *path)
{
    assert(*path++ == '/');    /* 目前只支持从根目录开始的路径 */
    Inode    ino = ext2_root_inode;
    int    ino_nr, len;
    char file[EXT2_NAME_LEN];

    while((len=get_filename_from_path(path, file)) != 0)
    {
#if 0
        printk("file: %s ", file);
#endif
        ino_nr = ext2_search_file(file, &ino, &ino);

        path += len;

        if(*path != 0)
            path++;
    }
#if 0
    printk("ino_nr:%d\n", ino_nr);
#endif
    return ino_nr;
}

#if 0
/*
 * 需要自己释放内存
 */
char *ext2_loadfile(const char *path, size_t *file_size)
{
    char *buf;
    buf = load_inode_content(&inode);
    *file_size = inode.i_size;

    return buf;
}
#endif

static void ext2_print_group_descriptor(pGroupDesc p)
{
    printk("block bitmap:%d inode bitmap:%d inode table:%d\n",
        p->bg_block_bitmap,
        p->bg_inode_bitmap,
        p->bg_inode_table);
}

static void ext2_print_inode(pInode p)
{
    printk("i_mode:%04x i_size:%d i_blocks:%d\n",
        p->i_mode, p->i_size, p->i_blocks);
}

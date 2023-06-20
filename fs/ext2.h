/*
 *--------------------------------------------------------------------------
 *   File Name: ext2.h
 *
 * Description: 当然.几乎来自Linux 内核.
 *
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *
 *     Version:    1.0
 * Create Date: Fri Dec 26 22:43:43 2008
 * Last Update: Fri Dec 26 22:43:43 2008
 *
 *--------------------------------------------------------------------------
 */

#ifndef _EXT2_H
#define _EXT2_H

#include <types.h>

#define EXT2_SUPER_MAGIC 0xEF53

#define EXT2_SB_OFFSET 1024

#define EXT2_BAD_INO 1
#define EXT2_ROOT_INO 2
#define EXT2_BOOT_LOADER_INO 5
#define EXT2_UNDEL_DIR_INO 6

#define EXT2_MIN_BLOCK_SIZE 1024
#define EXT2_MAX_BLOCK_SIZE 4096
#define EXT2_MIN_BLOCK_LOG_SIZE 10

#define EXT2_SB (&ext2_fs.ext2_sb)
#define EXT2_GD (ext2_fs.ext2_gd)

unsigned long ext2_block_size();
#define EXT2_BLOCK_SIZE ext2_block_size()
// #define EXT2_BLOCK_SIZE        (EXT2_MIN_BLOCK_SIZE << (EXT2_SB)->s_log_block_size)

#define EXT2_SECT_PER_BLOCK (EXT2_BLOCK_SIZE / 512)

#define EXT2_BLOCK_SIZE_BITS ((EXT2_SB)->s_log_block_size + 10)
#define EXT2_INODE_SIZE ((EXT2_SB)->s_inode_size)
#define EXT2_INODES_PER_BLOCK (EXT2_BLOCK_SIZE / EXT2_INODE_SIZE)
#define EXT2_FIRST_INO ((EXT2_SB)->s_first_ino)
/*
 * 表示第一个块号. 因为SuperBlock总是从第三个扇区开始的所以如果块的大小
 * 是1024的话SuperBlock的块号是1.而如果块的大小是2048或4096则SuperBlock
 * 的块号是0
 */
#define EXT2_FIRST_BLOCK_ID (EXT2_BLOCK_SIZE == 1024)

#define EXT2_BLOCKS_PER_GROUP ((EXT2_SB)->s_blocks_per_group)
#define EXT2_DESC_PER_BLOCK ((EXT2_SB)->s_desc_per_block)
#define EXT2_INODES_PER_GROUP ((EXT2_SB)->s_inodes_per_group)
#define EXT2_INODES_COUNT ((EXT2_SB)->s_inodes_count)

/*
 * ------------------------------------------------------------------------
 *  EXT2 FILE SYSTEM PART
 * ------------------------------------------------------------------------
 */
// https://www.nongnu.org/ext2-doc/ext2.html
// 关于s_first_data_block
// 32bit value identifying the first data block, in other word the id of the block containing the superblock structure.
// Note that this value is always 0 for file systems with a block size larger than 1KB, and always 1 for file systems
// with a block size of 1KB. The superblock is always starting at the 1024th byte of the disk, which normally happens to
// be the first byte of the 3rd sector.
typedef struct ext2_superblock {
    u32 s_inodes_count;      /* Inodes count */
    u32 s_blocks_count;      /* Blocks count */
    u32 s_r_blocks_count;    /* Reserved blocks count */
    u32 s_free_blocks_count; /* Free blocks count */
    u32 s_free_inodes_count; /* Free inodes count */
    u32 s_first_data_block;  /* First Data Block */
    u32 s_log_block_size;    /* Block size */
    u32 s_log_frag_size;     /* Fragment size */
    u32 s_blocks_per_group;  /* # Blocks per group */
    u32 s_frags_per_group;   /* # Fragments per group */
    u32 s_inodes_per_group;  /* # Inodes per group */
    u32 s_mtime;             /* Mount time */
    u32 s_wtime;             /* Write time */
    u16 s_mnt_count;         /* Mount count */
    u16 s_max_mnt_count;     /* Maximal mount count */
    u16 s_magic;             /* Magic signature */
    u16 s_state;             /* File system state */
    u16 s_errors;            /* Behaviour when detecting errors */
    u16 s_minor_rev_level;   /* minor revision level */
    u32 s_lastcheck;         /* time of last check */
    u32 s_checkinterval;     /* max. time between checks */
    u32 s_creator_os;        /* OS */
    u32 s_rev_level;         /* Revision level */
    u16 s_def_resuid;        /* Default uid for reserved blocks */
    u16 s_def_resgid;        /* Default gid for reserved blocks */
    /*
     * These fields are for EXT2_DYNAMIC_REV superblocks only.
     *
     * Note: the difference between the compatible feature set and
     * the incompatible feature set is that if there is a bit set
     * in the incompatible feature set that the kernel doesn't
     * know about, it should refuse to mount the filesystem.
     *
     * e2fsck's requirements are more strict; if it doesn't know
     * about a feature in either the compatible or incompatible
     * feature set, it must abort and not try to meddle with
     * things it doesn't understand...
     */
    u32 s_first_ino;              /* First non-reserved inode */
    u16 s_inode_size;             /* size of inode structure */
    u16 s_block_group_nr;         /* block group # of this superblock */
    u32 s_feature_compat;         /* compatible feature set */
    u32 s_feature_incompat;       /* incompatible feature set */
    u32 s_feature_ro_compat;      /* readonly-compatible feature set */
    u8 s_uuid[16];                /* 128-bit uuid for volume */
    char s_volume_name[16];       /* volume name */
    char s_last_mounted[64];      /* directory where last mounted */
    u32 s_algorithm_usage_bitmap; /* For compression */
    /*
     * Performance hints.  Directory preallocation should only
     * happen if the EXT2_COMPAT_PREALLOC flag is on.
     */
    u8 s_prealloc_blocks;     /* Nr of blocks to try to preallocate*/
    u8 s_prealloc_dir_blocks; /* Nr to preallocate for dirs */
    u16 s_padding1;
    /*
     * Journaling support valid if EXT3_FEATURE_COMPAT_HAS_JOURNAL set.
     */
    u8 s_journal_uuid[16]; /* uuid of journal superblock */
    u32 s_journal_inum;    /* inode number of journal file */
    u32 s_journal_dev;     /* device number of journal file */
    u32 s_last_orphan;     /* start of list of inodes to delete */
    u32 s_hash_seed[4];    /* HTREE hash seed */
    u8 s_def_hash_version; /* Default hash version to use */
    u8 s_reserved_char_pad;
    u16 s_reserved_word_pad;
    u32 s_default_mount_opts;
    u32 s_first_meta_bg; /* First metablock block group */
    u32 s_reserved[190]; /* Padding to the end of the block */
} ext2_sb_t;

typedef struct ext2_group_descriptor {
    u32 bg_block_bitmap;
    u32 bg_inode_bitmap;
    u32 bg_inode_table;
    u16 bg_free_blocks_count;
    u16 bg_free_inodes_count;
    u16 bg_used_dirs_count;
    u16 bg_pad;
    u32 bg_reserved[3];
} ext2_gd_t;

#define EXT2_NDIR_BLOCKS (12)
#define EXT2_IND_BLOCK (EXT2_NDIR_BLOCKS)
#define EXT2_DIND_BLOCK (EXT2_IND_BLOCK + 1)
#define EXT2_TIND_BLOCK (EXT2_DIND_BLOCK + 1)
#define EXT2_N_BLOCKS (EXT2_TIND_BLOCK + 1)

typedef struct ext2_inode {
    u16 i_mode;
    u16 i_uid;
    u32 i_size;
    u32 i_atime;
    u32 i_ctime;
    u32 i_mtime;
    u32 i_dtime;
    u16 i_gid;
    u16 i_links_count;
    u32 i_blocks;
    u32 i_flags;
    u32 i_osd1;
    u32 i_block[EXT2_N_BLOCKS];
    u32 i_generation;
    u32 i_file_acl;
    u32 i_dir_acl;
    u32 i_faddr;
    u8 i_osd2[12];
} ext2_inode_t;

#define EXT2_NAME_LEN 255
typedef struct ext2_dir_ent {
    u32 inode;
    u16 rec_len;
    u8 name_len;
    u8 file_type;
    char name[EXT2_NAME_LEN];
} ext2_dirent_t;

/*
 * Ext2 目录类型.
 * 到目前为止只有低3位有效.
 */
enum {
    EXT2_FT_UNKNOWN,
    EXT2_FT_REG_FILE,
    EXT2_FT_DIR,
    EXT2_FT_CHRDEV,
    EXT2_FT_BLKDEV,
    EXT2_FT_FIFO,
    EXT2_FT_SOCK,
    EXT2_FT_SYMLINK,
    EXT2_FT_MAX
};

#define EXT2_DIR_PAD 4
#define EXT2_DIR_ROUND (EXT2_DIR_PAD - 1)
#define EXT2_DIR_REC_LEN(name_len) (((name_len) + 8 + EXT2_DIR_ROUND) & ~EXT2_DIR_ROUND)
#define EXT2_MAX_REC_LEN ((1 << 16) - 1)

void ext2_read_inode(unsigned int ino, ext2_inode_t *inode);
void ext2_read_file(const ext2_inode_t *inode, char *buf);
void ext2_read_data(const ext2_inode_t *inode, unsigned int offset, size_t size, char *buf);

#endif  //_EXT2_H

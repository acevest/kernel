/*
 *--------------------------------------------------------------------------
 *   File Name: stat.h
 *
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Fri Feb 12 18:01:27 2010
 *
 * Description: none
 *
 *--------------------------------------------------------------------------
 */

#ifndef _STAT_H
#define _STAT_H

typedef struct stat {
    unsigned long st_dev;
    unsigned long st_ino;
    unsigned short st_mode;
    unsigned short st_nlink;
    unsigned short st_uid;
    unsigned short st_gid;
    unsigned long st_rdev;
    unsigned long st_size;
    unsigned long st_blksize;
    unsigned long st_blocks;
    unsigned long st_atime;
    unsigned long st_atime_nsec;
    unsigned long st_mtime;
    unsigned long st_mtime_nsec;
    unsigned long st_ctime;
    unsigned long st_ctime_nsec;
    unsigned long __unused4;
    unsigned long __unused5;
} Stat, *pStat;

// 8进制
#define S_IFMT 00170000
#define S_IFREG 0010000
#define S_IFDIR 0020000
#define S_IFBLK 0030000
#define S_IFCHR 0040000
#define S_IFLNK 0050000

#define S_ISUID 0004000
#define S_ISGID 0002000
#define S_ISVTX 0001000

#define S_ISREG(x) (((x) & S_IFMT) == S_IFREG)
#define S_ISDIR(x) (((x) & S_IFMT) == S_IFDIR)
#define S_ISBLK(x) (((x) & S_IFMT) == S_IFBLK)
#define S_ISCHR(x) (((x) & S_IFMT) == S_IFCHR)
#define S_ISLNK(x) (((x) & S_IFMT) == S_IFLNK)

// user权限
#define S_IURWX 00700
#define S_IUSRR 00400
#define S_IUSRW 00200
#define S_IUSRX 00100

// group权限
#define S_IGRWX 00070
#define S_IGRPR 00040
#define S_IGRPW 00020
#define S_IGRPX 00010

// other权限
#define S_IORWX 00007
#define S_IOTHR 00004
#define S_IOTHW 00002
#define S_IOTHX 00001

#endif  //_STAT_H

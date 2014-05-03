/*
 *--------------------------------------------------------------------------
 *   File Name: exec.c
 * 
 *      Author: Zhao Yanbai [zhaoyanbai@126.com]
 *              Sat Feb 20 21:12:30 2010
 * 
 * Description: none
 * 
 *--------------------------------------------------------------------------
 */

#include <syscall.h>
#include <types.h>
#include <fcntl.h>
#include <assert.h>
#include <stat.h>
#include <sched.h>
#include <memory.h>
#include <elf.h>

int sysc_exec(const char *path, char *const argv[])
{
    assert(argv == NULL);    // unsupport now

    int fd;
    int i;

    fd = sysc_open(path, O_RDONLY, 0);
    if(fd == -1)
    {
        panic("can not find file");
    }

    int    filesz;
    Stat    stat;
    char    *buf;

    sysc_stat(fd, &stat);
    filesz    = stat.st_size;
    if(stat.st_size <=0 || stat.st_size>MAX_SUPT_FILE_SIZE)
    {
        printk("file %s is not exist\n", path);
        return -ENOENT;
    }
    buf = (void*)kmalloc_old(filesz);
    sysc_read(fd, buf, filesz);

#if 0    
    for(i=0; i<filesz; i++)
        printk("%02x ", (unsigned char)buf[i]);
#endif

    pElf32_Ehdr ehdr = (pElf32_Ehdr) buf;
    //assert(strncmp(ELFMAG, ehdr->e_ident, sizeof(ELFMAG)-1) == 0);
    if(strncmp(ELFMAG, ehdr->e_ident, sizeof(ELFMAG)-1) != 0)
    {
        printk("file %s can not execute\n", path);
        kfree_old(buf);
        return -ENOEXEC;
    }
    //printk("Entry: %08x phnum:%d\n", ehdr->e_entry, ehdr->e_phnum);
    
    int size = 0;
    char *pv = NULL;    // phdr 中第一个的VirtAddr
    for(i=0; i<ehdr->e_phnum; i++)
    {
        pElf32_Phdr phdr;
        phdr = (pElf32_Phdr)(buf+ehdr->e_phoff+(i*ehdr->e_phentsize));
        //printk(" %d %08x\n", i, phdr->p_type);
        if(phdr->p_type == PT_LOAD)
        {
            size += phdr->p_memsz;
            if(i==0)
                pv = (char *)phdr->p_vaddr;
        }
    }

    char *exe = (char *) kmalloc_old(size);
    for(i=0; i<ehdr->e_phnum; i++)
    {
        pElf32_Phdr phdr;
        phdr = (pElf32_Phdr)(buf+ehdr->e_phoff+(i*ehdr->e_phentsize));
        if(phdr->p_type != PT_LOAD)
            continue;
#if 0
        printk("%08x ", exe+phdr->p_vaddr-pv);
        printk("p_offset:%d\n", phdr->p_offset);

        int j;
        for(j=0; j<100;/*phdr->p_filesz*/ j++)
            printk("%02x ", *((char *)(buf+phdr->p_offset)+j));
#endif
        if(phdr->p_filesz != 0)
        {
            memcpy((void*)(exe+phdr->p_vaddr-pv),
                (void*)(buf+phdr->p_offset),
                phdr->p_filesz);
        }


#if 0
        u32    *pd =(u32*)pa2va(current->cr3);
        u32    *pt;
        u32    npd_min, npd_max, npt;
        u32    pde, pte;
        u32    vaddr = phdr->p_vaddr;
        npd_min = get_npd(vaddr);
        npd_max = get_npd(vaddr+phdr->p_memsz);
        u32    npd;

        for(npd=npd_min; npd<=npd_max; npd++)
        {
            void *tmp = get_phys_pages(1);
            pd[npd] = tmp | 7;
        }
#endif
    }


    /*
     *  因为目前文件支持最大为12*EXT2_BLOCK_SIZE
     *  即12K~48K之间
     *  所以就以一个页目录项来简化处理
     */
    u32    *pd = (u32*) pa2va(current->cr3);
    u32    *pt;
    u32    pa_exe;
    u32    npd, npt;
    
    pa_exe    = va2pa(exe);
    npd    = get_npd(ehdr->e_entry);
    pt    = get_phys_pages(1);
    if(pt == NULL)
        panic("out of memory");
    //printk("npd: %d pt:%08x\n", npd, pt);
    memset(pa2va(pt), 0, PAGE_SIZE);
    pd[npd]    = (u32) pt | 7;
    pt = pa2va(pt);
    for(i=0; i<ehdr->e_phnum; i++)
    {
        pElf32_Phdr phdr;
        phdr = (pElf32_Phdr)(buf+ehdr->e_phoff+(i*ehdr->e_phentsize));
        if(phdr->p_type != PT_LOAD)
            continue;

        u32    npt_min, npt_max;

        npt_min = get_npt(phdr->p_vaddr);
        npt_max = get_npt(phdr->p_vaddr+phdr->p_memsz);
        //printk("npt_min:%d npt_max:%d\n", npt_min, npt_max);
        int j;
        for(j=npt_min; j<=npt_max; j++)
        {
            pt[j] = (u32)(pa_exe | 7);    // 对于.text不能这样
            //printk("pt[j] :%08x\n", pt[j]);
            pa_exe = PAGE_SIZE+pa_exe;
        }
    }
    
    //printk("exe : %08x cr3:%08x\n", exe, pd);


    /* 准备内核栈的数据并从ret_from_fork返回 */
    pt_regs_t *    regs    = ((pt_regs_t *)(TASK_SIZE+(unsigned long)current)) - 1;
    extern void ret_from_fork();
    memset((void*)regs, 0, sizeof(pt_regs_t));
    regs->ss    = SELECTOR_USER_DS;
    regs->ds    = SELECTOR_USER_DS;
    regs->es    = SELECTOR_USER_DS;
    regs->fs    = SELECTOR_USER_DS;
    regs->gs    = SELECTOR_USER_DS;
    regs->esp    = (KRNLADDR-4*sizeof(unsigned long));
    regs->eflags    = 0x200;
    regs->cs    = SELECTOR_USER_CS;
    regs->eip    = (unsigned long)ehdr->e_entry;
    current->esp    = (unsigned long) regs;
    current->eip    = (unsigned long)ret_from_fork;

#if 0    /* 写完之后发现貌似不用 */
    /* 准备用户栈数据 */
    /* 先找到用户栈的位置 */
    u32 pde = pd[get_npd(KRNLADDR)-1] & PAGE_MASK;
    pt = pa2va(pde);
    u32 *stack = (u32*)pa2va(pt[1023]);
    stack[1023] = 0x00;
    stack[1022] = 0x00;    /* ebp */
    stack[1021] = 0x00;    /* edx */
    stack[1020] = 0x00;    /* ecx */
    printk("stack pt: %08x pde:%08x %08x %08x\n",
        pt, pde, pd[get_npd(KRNLADDR)-1]);
#endif
    kfree_old(buf);


    //printk("eip: %08x \n", regs->eip);

    load_cr3(current);

    return 0;
}

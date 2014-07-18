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
#include <mm.h>
#include <elf.h>
#include <fs.h>
#include <ext2.h>

extern void *syscall_exit;

int sysc_exec(const char *path, char *const argv[])
{
    assert(argv == NULL);    // unsupport now

    unsigned int ino = namei(path);
    if(ino == 0)
        return -ENOENT;

    ext2_inode_t inode;

    ext2_read_inode(ino, &inode);

    //void *buf = (void*)kmalloc(inode.i_size, 0);
    void *buf = (void *) alloc_pages(0, 5);
    assert(buf != 0);

    printk("exec buf %08x \n", buf);
    printd("begin read elf\n");
    ext2_read_file(&inode, buf);
    printd("end read elf\n");


    pElf32_Ehdr ehdr = (pElf32_Ehdr) buf;
    //assert(strncmp(ELFMAG, ehdr->e_ident, sizeof(ELFMAG)-1) == 0);
    if(strncmp(ELFMAG, ehdr->e_ident, sizeof(ELFMAG)-1) != 0)
    {
        printk("file %s can not execute\n", path);
        kfree(buf);
        return -ENOEXEC;
    }
    //printk("Entry: %08x phnum:%d\n", ehdr->e_entry, ehdr->e_phnum);
    
    int size = 0;
    int i;
    for(i=0; i<ehdr->e_phnum; i++)
    {
        pElf32_Phdr phdr;
        phdr = (pElf32_Phdr)(buf+ehdr->e_phoff+(i*ehdr->e_phentsize));

        printk("Type %08x Off %08x Va %08x Pa %08x Fsz %08x Mmsz %08x\n",
            phdr->p_type, phdr->p_offset, phdr->p_vaddr, phdr->p_paddr,
            phdr->p_filesz, phdr->p_memsz);

        if(phdr->p_type == PT_LOAD)
        {
            if(phdr->p_offset > 0 && phdr->p_memsz > 0)
            {
                size = ALIGN(phdr->p_offset + phdr->p_memsz, 0x1000);
            }
        }
    }

    printk("ELF MEM SIZE %u\n", size);

    char *exe = (char *) kmalloc(size, 0);
    assert(exe != 0);
    printk("EXE ADDR %08x\n", exe);
    for(i=0; i<ehdr->e_phnum; i++)
    {
        pElf32_Phdr phdr;
        phdr = (pElf32_Phdr)(buf+ehdr->e_phoff+(i*ehdr->e_phentsize));
        if(phdr->p_type != PT_LOAD)
            continue;
        if(phdr->p_filesz != 0)
        {
            memcpy((void*)(exe+phdr->p_offset), (void*)(buf+phdr->p_offset), phdr->p_filesz);
        }
    }


    /*
     *  因为目前文件支持最大为12*EXT2_BLOCK_SIZE
     *  即12K~48K之间
     *  所以就以一个页目录项来简化处理
     */
    u32    *pd = (u32*) current->cr3;
    u32    *pt;
    u32    pa_exe;
    u32    npd, npt;
    int    c;
    pa_exe  = va2pa(exe);
    npd     = get_npd(ehdr->e_entry);
    npt     = get_npt(ehdr->e_entry);
    pt      = (u32*)va2pa(alloc_one_page(0));
    if(pt == NULL)
        panic("out of memory");

    //printk("npd: %d pt:%08x\n", npd, pt);
    memset(pa2va(pt), 0, PAGE_SIZE);
    pd[npd]    = (u32) pt | 7;
    pt = pa2va(pt);
    for(i=npt, c=0; i<1024; i++, c++)
    {
        pt[i] = va2pa(PAGE_ALIGN((unsigned long)exe)) + c * PAGE_SIZE;
        pt[i] |= 7;
    }
    
    load_cr3(current);
    printk("exe : %08x cr3:%08x\n", exe, pd);

    pt_regs_t *regs = ((pt_regs_t *)(TASK_SIZE+(unsigned long)current)) - 1;
    memset((void*)regs, 0, sizeof(pt_regs_t));
    regs->ss    = SELECTOR_USER_DS;
    regs->ds    = SELECTOR_USER_DS;
    regs->es    = SELECTOR_USER_DS;
    regs->fs    = SELECTOR_USER_DS;
    regs->gs    = SELECTOR_USER_DS;
    regs->esp   = (KRNLADDR-4*sizeof(unsigned long));
    regs->eflags= 0x200;
    regs->cs    = SELECTOR_USER_CS;
    regs->eip   = (unsigned long)ehdr->e_entry;
    regs->edx   = regs->eip;
    regs->ecx   = (0xC0000000 - 16);

    kfree(buf);

    asm("movl $0, %%eax; movl %%ebx,%%ebp; movl %%ebp,%%esp;jmp syscall_exit;"::"b"((unsigned long)(regs)));

    return 0;
}

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

#include <assert.h>
#include <elf.h>
#include <ext2.h>
#include <fcntl.h>
#include <fs.h>
#include <mm.h>
#include <page.h>
#include <sched.h>
#include <stat.h>
#include <syscall.h>
#include <types.h>

extern void *syscall_exit;

void put_paging(unsigned long vaddr, unsigned long paddr, unsigned long flags) {
    assert(PAGE_ALIGN(vaddr) == vaddr);
    assert(PAGE_ALIGN(paddr) == paddr);

    unsigned int npde = get_npde(vaddr);
    unsigned int npte = get_npte(vaddr);

    pde_t *page_dir = (pde_t *)pa2va(current->cr3);
    pte_t *page_table = (pte_t *)PAGE_ALIGN(page_dir[npde]);

    if (page_table == 0) {
        page_table = (pte_t *)alloc_one_page(0);
        memset(page_table, 0, PAGE_SIZE);
        page_table = (pte_t *)va2pa(page_table);
        assert(page_table != 0);
    }

    page_dir[npde] = (unsigned long)page_table | flags | PAGE_P | PAGE_WR;
    page_table = pa2va(page_table);
    page_table[npte] = paddr | flags;
}

int sysc_exec(const char *path, char *const argv[]) {
    assert(argv == NULL);  // unsupport now

    unsigned int ino = namei(path);
    if (ino == 0) return -ENOENT;

    ext2_inode_t inode;

    ext2_read_inode(ino, &inode);

    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)alloc_one_page(0);
    assert(ehdr != 0);
    ext2_read_data(&inode, 0, PAGE_SIZE, (char *)ehdr);
    printk("%08x\n", *((unsigned long *)ehdr->e_ident));
    assert(strncmp(ELFMAG, ehdr->e_ident, sizeof(ELFMAG) - 1) == 0);
    printk("Elf Entry: %08x\n", ehdr->e_entry);

    int i, j;
    for (i = 0; i < ehdr->e_phnum; ++i) {
        Elf32_Phdr *phdr;
        phdr = (Elf32_Phdr *)(((unsigned long)ehdr) + ehdr->e_phoff + (i * ehdr->e_phentsize));

        printk("Type %08x Off %08x Va %08x Pa %08x Fsz %08x Mmsz %08x\n", phdr->p_type, phdr->p_offset, phdr->p_vaddr, phdr->p_paddr, phdr->p_filesz,
               phdr->p_memsz);

        unsigned long vaddr = phdr->p_vaddr;
        unsigned long offset = phdr->p_offset;
        unsigned long mmsz = phdr->p_memsz;
        unsigned long filesz = phdr->p_filesz;

        if (phdr->p_type != PT_LOAD) continue;

        assert(mmsz >= filesz);

        unsigned int pgcnt = (mmsz + PAGE_SIZE - 1) / PAGE_SIZE;
        unsigned int blkcnt = (filesz + EXT2_BLOCK_SIZE - 1) / EXT2_BLOCK_SIZE;

        void *buf = kmalloc(pgcnt * PAGE_SIZE, PAGE_SIZE);
        assert(PAGE_ALIGN(buf) == (unsigned long)buf);
        assert(buf != 0);

        printk("vvvbufsz %08x datasz %08x\n", pgcnt * PAGE_SIZE, blkcnt * EXT2_BLOCK_SIZE);

        if (filesz > 0) ext2_read_data(&inode, offset, blkcnt * EXT2_BLOCK_SIZE, buf);

        for (j = 0; j < pgcnt; ++j) {
            put_paging(vaddr + j * PAGE_SIZE, (unsigned long)(va2pa(buf)) + j * PAGE_SIZE, 7);
        }
    }

    load_cr3(current);

    disable_irq();

    pt_regs_t *regs = ((pt_regs_t *)(TASK_SIZE + (unsigned long)current)) - 1;
#if 1
    memset((void *)regs, 0, sizeof(pt_regs_t));
    regs->ss = SELECTOR_USER_DS;
    regs->ds = SELECTOR_USER_DS;
    regs->es = SELECTOR_USER_DS;
    regs->fs = SELECTOR_USER_DS;
    regs->gs = SELECTOR_USER_DS;
    regs->esp = (KRNLADDR - 4 * sizeof(unsigned long));
    regs->eflags = 0x200;
    regs->cs = SELECTOR_USER_CS;
#endif
    regs->eip = (unsigned long)ehdr->e_entry;
    regs->edx = regs->eip;
    regs->ecx = KRNLADDR;  //(0xC0000000 - 16);

    // kfree(buf);

    free_pages((unsigned long)ehdr);

    // TODO FIXME
    // asm("movl $0, %%eax; movl %%ebx,%%ebp; movl %%ebp,%%esp;jmp syscall_exit;" ::"b"((unsigned long)(regs)));

    return 0;
}

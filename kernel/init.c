#include <io.h>
#include <sched.h>
#include <types.h>
#include <page.h>
#include <stdio.h>
#include <system.h>
#include <syscall.h>
#include <processor.h>
#include <irq.h>
#include <fcntl.h>
#include <stat.h>
#include <init.h>


void    root_task_entry();
void    setup_kernel();

TSS    tss;
System    system;
Desc    idt[NIDT];
Desc    gdt[NGDT];

char __initdata kernel_init_stack[KRNL_INIT_STACK_SIZE] __attribute__ ((__aligned__(PAGE_SIZE)));

int KernelEntry()
{
    setup_kernel();

#if 0
    char *root_task_user_space_stack = (char *) alloc_pages(0, 0);

    asm("movl   $0x23,%%eax;        \
        movw    %%ax,%%ds;          \
        movw    %%ax,%%es;          \
        movw    %%ax,%%fs;          \
        movw    %%ax,%%gs;          \
        pushl   $0x23;              \
        pushl   %%ebx;              \
        pushl   $0x282;             \
        pushl   $0x1B;              \
        leal    root_task_entry,%%eax;    \
        pushl   %%eax;              \
        iret;"::"b"(root_task_user_space_stack+PAGE_SIZE));
#else
#if 0
    asm("xorl  %eax, %eax; \
        sti;\
        pushfl;  \
        movw  %cs, %ax;   \
        pushl   %eax;\
        leal    root_task_entry,%eax;    \
        pushl   %eax;              \
        iret;"::"b"(root_task.cr3 + TASK_SIZE));
#endif
#endif

    return 0; /* never come to here */
}

#if 0
void root_task_entry()
{
    pid_t pid;
    pid = fork();
    if(pid < 0)
    {
        printf("failed create child\n");
        while(1);
    }
    else if(pid == 0)
    {
        execv("/bin/sh", NULL);
        while(1);
    }
    else
    {
        while(1)
        {

        }
    }
}
#else
void root_task_entry()
{
    pt_regs_t regs;
    int pid = do_fork(regs, 0);

    printk("pid is %d\n", pid);

    if(pid > 0)
    {
        while(1)
        {
            asm("hlt;");
            sysc_test();
            //syscall0(SYSC_TEST);
        }
    }
    else if(pid == 0)
    {

    }
    else
    {
        printk("err\n");
    }
    //pid_t pid;
/*
    int fd = open("/boot/grub/grub.conf", O_RDONLY);
    //int fd = open("/bin/hw", O_RDONLY);
    printf("FD: %d\n", fd);
    char    buf[1024];
    int    filesz, i;
    Stat    stat;
    fstat(fd, &stat);
    filesz = stat.st_size;
    printf("file size: %d bytes\n", filesz);
    read(fd, buf, filesz);
    for(i=0; i<filesz; i++)
        printf("%c", buf[i]);
    execv("/bin/hw", NULL);
    while(0)
    {
        printf("r");
        int d=100000; while(d--);
    }
*/


//#define    SHOW    

    pid = fork();
#ifdef    SHOW
    printf("PID: %d\n", pid);
#endif
    if(pid < 0)
    {
        printf("failed create child\n");
        while(1);
    }
    else if(pid == 0)
    {
        pid_t ccid;
        ccid = fork();

#ifdef    SHOW
        printf("CCID: %d\n", ccid);
#endif

        if(ccid < 0)
        {
            printf("child faild to create child\n");
            while(1);
        }
        else if(ccid == 0)
        {
            execv("/bin/sh", NULL);
            while(1)
            {
#ifdef    SHOW
                printf("a");
#endif
                delay(100);
            }
        }
        else
        {
            while(1)
            {
#ifdef    SHOW
                printf("b");
#endif
                delay(200);
            }
        }
    }
    else
    {
        pid_t pcid;
        pcid = fork();
    
#ifdef    SHOW
        printf("PCID: %d\n", pcid);
#endif
        if(pcid < 0)
        {
            printf("parent faild to create child\n");
            while(1);
        }
        else if(pcid == 0)
        {
            while(1)
            {
#ifdef    SHOW
                printf("c");
#endif
                delay(300);
            }
        }
        else
        {
            while(1)
            {
#if 0
                int k;
                extern    void    ParseKbdInput(int k);
                k = read_kbd();
                ParseKbdInput(k);
#else
#ifdef    SHOW
                printf("d");
#endif
                delay(400);
#endif
            }
        }
    }
}
#endif

#include<io.h>
#include<sched.h>
#include<types.h>
#include<page.h>
#include<stdio.h>
#include<system.h>
#include<syscall.h>
#include<processor.h>
#include<irq.h>
#include<fcntl.h>
#include<stat.h>

#define	KRNL_STACK_SIZE	4096

extern void	root_task();
extern void	setup_kernel();

TSS	tss;
System	system;

static char	kernel_stack[KRNL_STACK_SIZE] __attribute__ ((__aligned__(PAGE_SIZE)));

int KernelEntry()
{
	asm(	"movl $kernel_stack,%%esp;"
		"addl %%eax,%%esp;"
		::"a"(KRNL_STACK_SIZE));

	setup_kernel();

	asm("	movl	$0x23,%%eax;		\
		movw	%%ax,%%ds;		\
		movw	%%ax,%%es;		\
		movw	%%ax,%%fs;		\
		movw	%%ax,%%gs;		\
		pushl	$0x23;			\
		pushl	%%ebx;			\
		pushl	$0x282;			\
		pushl	$0x1B;			\
		leal	root_task,%%eax;	\
		pushl	%%eax;			\
		iret;"::"b"(KRNLADDR));
	return 0;
}

#if 0
void root_task()
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
void root_task()
{
	pid_t pid;
/*
	int fd = open("/boot/grub/grub.conf", O_RDONLY);
	//int fd = open("/bin/hw", O_RDONLY);
	printf("FD: %d\n", fd);
	char	buf[1024];
	int	filesz, i;
	Stat	stat;
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


//#define	SHOW	

	pid = fork();
#ifdef	SHOW
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

#ifdef	SHOW
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
#ifdef	SHOW
				printf("a");
#endif
				delay(100);
			}
		}
		else
		{
			while(1)
			{
#ifdef	SHOW
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
	
#ifdef	SHOW
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
#ifdef	SHOW
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
				extern	void	ParseKbdInput(int k);
				k = read_kbd();
				ParseKbdInput(k);
#else
#ifdef	SHOW
				printf("d");
#endif
				delay(400);
#endif
			}
		}
	}
}
#endif

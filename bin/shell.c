/*
 *--------------------------------------------------------------------------
 *   File Name:	shell.c
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 			Wed Feb 24 17:47:22 2010
 * 
 * Description:	none
 * 
 *--------------------------------------------------------------------------
 */
#include<stdio.h>
#include<types.h>
#include<string.h>
#define	CMD_SIZE	256
char cmd[CMD_SIZE];

void get_cmd()
{
	int i;

	i=0;
	printf("#");
	while(1)
	{
		int k;
		char ch;
		extern char ParseKbdInput(int k);
		extern unsigned char read_kbd();
reinput:
		k = read_kbd();
		ch = ParseKbdInput(k);
		if(ch == -1)
			continue;
		
		if(ch == '\b')
		{
            i = --i < 0 ? 0 : i;
			cmd[i] = 0;
			continue;
		}

		if(ch == '\n')
		{
			cmd[i++] = 0;
			break;
		}

		cmd[i++] = ch;

		if(i == CMD_SIZE - 1)
		{
			printf("shell buffer is full..."
				"reset buffer...\n");
			i = 0;
		}
	}

    int len = strlen(cmd);
    int j;
    int flag = 0;
    for(j = len - 1; j>=0; j--) {
        if(cmd[j] != '\n') {
            flag = 1;
            break;
        }
    }

    if(flag == 0) {
        i = 0;
        goto reinput;
    }

	cmd[CMD_SIZE-1] = 0;
}

int shell()
{
	char buf[CMD_SIZE];

	while(1)
	{
		get_cmd();

		//printf("\nCMD: %s\n", cmd);

		if(cmd[0] == 0) continue;
		if(cmd[0] != '/')
		{
			strcpy(buf, cmd);
			strcpy(cmd, "/bin/");
			strcat(cmd, buf);
		}
		
		pid_t pid;
		pid = fork();

		if(pid<0)
		{
			printf("shit happens in shell\n");
			while(1);
		}
		else if(pid == 0)
		{
			execv(cmd, NULL);
		}

		int  i = 100000;
		while(i--);
	}



	return 0;
}

#if 0
int shell()
{
	pid_t pid;

	pid = fork();

	if(pid<0)
	{
		printf("shit happens in shell\n");
		while(1);
	}
	else if(pid == 0)
	{
		execv("/bin/hw", NULL);
	}
	else
	{

		while(1)
		{
			int k;
			char ch;
			extern char ParseKbdInput(int k);
			extern unsigned char read_kbd();
			//asm("xchg %bx, %bx");
			k = read_kbd();
			ch = ParseKbdInput(k);
			if(ch != -1)
			printf("<%c>",ch);
		}
	}


	return 0;
}
#endif

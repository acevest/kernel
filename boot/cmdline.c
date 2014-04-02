/*
 *--------------------------------------------------------------------------
 *   File Name:	cmdline.c
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 			Wed Feb 17 17:11:37 2010
 * 
 * Description:	none
 * 
 *--------------------------------------------------------------------------
 */
#include<fs.h>
#include<printk.h>
#include<system.h>
#include<string.h>
#include<stdlib.h>

void get_variable_value(const char *name, char *value);

void parse_root_dev()
{
	char	value[128];
	int	n;
	get_variable_value("root", value);

	/* 目前只支持通道一的一个硬盘 */
    /*
    printk("D:%s\n", value);
	assert(	value[0] == '(' &&
		value[1] == 'h' &&
		value[2] == 'd' &&
		value[3] == '0' &&
		value[4] == ',');
    */
    value[0] = '(';
    value[1] = 'h';
    value[2] = 'd';
    value[3] = '0';
    value[4] = ')';
    value[5] = ',';
    value[6] = '0';
    value[7] = '\0';
	n = atoi(value+5);

	system.root_dev = MAKE_DEV(DEV_MAJOR_HD, n+1);
}
void parse_debug()
{
	char value[128];
	int n;
	get_variable_value("debug", value);
	n = atoi(value);

	system.debug = (n != 0);
}


void parse_cmdline(char *cmdline)
{
	system.cmdline = cmdline;
	printk("cmdline: %s\n", system.cmdline);
	parse_root_dev();
	parse_debug();
#if 0
	get_variable_value("root", value);
	printk("root : %s\n", value, n);
	get_variable_value("debug", value);
	n = atoi(value);
	printk("debug : %s %d\n", value, n);
	while(1);
#endif
}


void get_variable_value(const char *name, char *value)
{
	char *p = system.cmdline;
	char buf[256];
	int  i;
	*value = 0;

	while(*p)
	{
		while(*p != ' ')
		{
			if(*p++ == 0)
				return;
		}
		p++;
		i = 0;
		while(*p != '=' && *p != 0)
			buf[i++] = *p++;
		if(*p++ == 0)
			return;
		buf[i] = 0;
		//printk("%s %s %d\n",buf, name, strcmp(buf, name));

		if(strcmp(buf, name) != 0)
		{
			while(*p != ' ' && *p != 0)
				p++;
			continue;
		}

		i = 0;
		while(*p != ' ' && *p != 0)
			value[i++] = *p++;
		value[i] = 0;
		//printk("DD %s", value);
		return ;
	}

}

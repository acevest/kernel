//=========================================================================
// vsprintf.c (C) Zhao Yanbai
// 	wed, 30 Jul 2008 14:47 +0800
// 	Add %012d %012x %12d %12x Support  Mon, 20 Jul 2009 19:30:34
// ========================================================================
#include"string.h"

char	*itoa(char *s, int n);
char	*itox(char *s, unsigned int n);

enum {
    ALIGN_RIGHT,
    ALIGN_LEFT
};

int write_buf(char *buf, const char *str, char fillch, int charcnt, int align)
{
    int len = strlen(str);
    int delta_char_cnt = charcnt - len;

    int s_pos = 0;
    int c_pos = len;

    if(ALIGN_RIGHT == align)
    {
        s_pos = delta_char_cnt > 0 ? delta_char_cnt : 0;
        c_pos = 0;
    }

    strcpy(buf + s_pos, str);

    int i = 0;
    for(i=0; i<delta_char_cnt; ++i)
    {
        buf[c_pos + i] = fillch;
    }

    return charcnt > len ? charcnt : len;
}

int vsprintf(char *buf, const char *fmt, char *args)
{
	char *p = buf;
	int char_cnt;
	char tmp[64];

	while(*fmt)
	{
		if( *fmt != '%' )
		{
			*p++ = *fmt++;
			continue;
		}

		fmt++;

        int align = ALIGN_RIGHT;
        if(*(fmt) == '-')
        {
            align = ALIGN_LEFT;
            ++fmt;
        }
        

        char char_fill = ' ';
		if(*(fmt) == '0' || *(fmt) == ' ')
        {
            char_fill = *(fmt);
            ++fmt;
        }

		char_cnt = 0;
		while(*(fmt) >= '0' && *(fmt) <= '9')
		{
			char_cnt += *(fmt) - '0';
			char_cnt *= 10;
			++fmt;
		}

        char_cnt /= 10;

		switch(*fmt)
		{
		case 'c':
			*p++ = *args;
			break;
		case 'd':
			itoa(tmp, *((int*)args));
            p += write_buf(p, tmp, char_fill, char_cnt, align);           
#if 0
			char_cnt -= strlen(tmp);
			if(char_cnt > 0)
			{
				while(char_cnt--)
				*p++ = char_fill;
			}
			strcpy(p, tmp);
			p += strlen(tmp);
#endif
			break;
		case 's':
            p += write_buf(p, (const char *)*((unsigned int *) args), char_fill, char_cnt, align);           
#if 0
			strcpy(p, (const char *)*((unsigned int *) args));
			p += strlen((const char *)*((unsigned int *) args));
#endif
			break;
		case 'x':
			itox(tmp, *((unsigned int *) args));
            p += write_buf(p, tmp, char_fill, char_cnt, align);           
#if 0
			char_cnt -= strlen(tmp);
			if(char_cnt > 0)
			{
				while(char_cnt--)
				 *p++ = char_fill;
			}
			strcpy(p, tmp);
			p += strlen(tmp);
#endif
			break;
		default:
			break;
		}
		args += 4;
		fmt++;
	}
	*p = 0;
}

char	*itoa(char *s, int n)
{
	int i;
	char tmp[64];
	if( n & 0x80000000 )
	{
		n	= ~n + 1;
		*s++	= '-';
	}
	i=0;
	do
	{
		tmp[i++]	= (n % 10) + '0';
		n		/= 10;
	}while(n);
	while(i) *s++=tmp[--i];
	*s = 0;
	
}

char	*itox(char *s, unsigned int n)
{
	char *p = s;
	char ch,i,flag = 0;
	
	//*p++	= '0';	*p++	= 'x';

	if(n==0){*p++='0';*p=0;return s;}
	for(i=28; i>=0; i-=4)
	{
		ch	= (n>>i) & 0x0F;
		if(ch >= 0)	ch += '0';
		if(ch > '9')	ch += 7;
		if(ch != '0')	flag = 1;
		if(ch != '0' || flag == 1)
			*p++ = ch;
	}
	*p = 0;
	return s;
}

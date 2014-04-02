/*
 *--------------------------------------------------------------------------
 *   File Name:	keyboard.h
 * 
 * Description:	none
 * 
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 
 *     Version:	1.0
 * Create Date: Thu Jul 16 18:39:57 2009
 * Last Update: Thu Jul 16 18:39:57 2009
 * 
 *--------------------------------------------------------------------------
 */
#include<system.h>
#include<syscall.h>
#include<stdio.h>
#include<io.h>
#define	EXT_KEY		0x80000000	/* None Print Key */
#define	L_SHIFT_DOWN	0x00000100
#define	R_SHIFT_DOWN	0x00000200
#define	L_CTRL_DOWN	0x00000400
#define	R_CTRL_DOWN	0x00000800
#define	L_ALT_DOWN	0x00001000
#define	R_ALT_DOWN	0x00002000
#define	L_SHIFT_UP	(~L_SHIFT_DOWN)
#define	R_SHIFT_UP	(~R_SHIFT_DOWN)
#define	L_CTRL_UP	(~L_CTRL_DOWN)
#define	R_CTRL_UP	(~R_CTRL_DOWN)
#define	L_ALT_UP	(~L_ALT_DOWN)
#define	R_ALT_UP	(~R_ALT_DOWN)
#define	SET_L_SHIFT_DOWN(key)	(key |= L_SHIFT_DOWN)
#define	SET_R_SHIFT_DOWN(key)	(key |= R_SHIFT_DOWN)
#define	SET_L_CTRL_DOWN(key)	(key |= L_CTRL_DOWN)
#define	SET_R_CTRL_DOWN(key)	(key |= R_CTRL_DOWN)
#define	SET_L_ALT_DOWN(key)	(key |= L_ALT_DOWN)
#define	SET_R_ALT_DOWN(key)	(key |= R_ALT_DOWN)
#define	SET_L_SHIFT_UP(key)	(key &= L_SHIFT_UP)
#define	SET_R_SHIFT_UP(key)	(key &= R_SHIFT_UP)
#define	SET_L_CTRL_UP(key)	(key &= L_CTRL_UP)
#define	SET_R_CTRL_UP(key)	(key &= R_CTRL_UP)
#define	SET_L_ALT_UP(key)	(key &= L_ALT_UP)
#define	SET_R_ALT_UP(key)	(key &= R_ALT_UP)
#define	IS_L_SHIFT_DOWN(key)	(key & L_SHIFT_DOWN)
#define	IS_R_SHIFT_DOWN(key)	(key & R_SHIFT_DOWN)
#define	IS_L_CTRL_DOWN(key)	(key & L_CTRL_DOWN)
#define	IS_R_CTRL_DOWN(key)	(key & R_CTRL_DOWN)
#define	IS_L_ALT_DOWN(key)	(key & L_ALT_DOWN)
#define	IS_R_ALT_DOWN(key)	(key & R_ALT_DOWN)

const unsigned char kbdCharTable[]={0,0,
'1','2','3','4','5','6','7','8','9','0','-','=','\b',0,
'q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\',
'z','x','c','v','b','n','m',',','.','/',0,0,0,' ',
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
const unsigned char kbdShiftCharTable[]={0,0,
'!','@','#','$','%','^','&','*','(',')','_','+','\b',0,
'Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
'A','S','D','F','G','H','J','K','L',':','"','~',0,'|',
'Z','X','C','V','B','N','M','<','>','?',0,0,0,' ',
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

/* Make Code */
#define	MC_BACKSPACE	0x0E
#define	MC_CTRL		0x1D
#define	MC_L_SHIFT	0x2A
#define	MC_R_SHIFT	0x36
#define	MC_ALT		0x38
#define	MC_CAPSLOCK	0x3A
#define	MC_DELETE	0x53
#define	BC_BACKSPACE	(0x80 | MC_BACKSPACE)
#define	BC_CTRL		(0x80 | MC_CTRL)
#define	BC_L_SHIFT	(0x80 | MC_L_SHIFT)
#define	BC_R_SHIFT	(0x80 | MC_R_SHIFT)
#define	BC_ALT		(0x80 | MC_ALT)
#define	BC_DELETE	(0x80 | MC_DELETE)
#define	BC_CAPSLOCK	(0x80 | MC_CAPSLOCK)

static unsigned char E0Flag = 0;

unsigned char read_kbd()
{
	char	k;
	int ret = -1;

	while(ret == -1)
	{
		syscall0(SYSC_READ_KBD);
		asm("":"=a"(ret));
	}

	k = (unsigned char) ret;


	return k;
}
/*
 * 貌似现在CapsLock不工作了
 */
unsigned int _parse_kbd(int ScanCode)
{
	//int ScanCode;
	static unsigned int key;
	//ScanCode = read_kbd();

	if(ScanCode == 0xE0)
	{
		E0Flag = 1;
		return -1;
	}

	if(ScanCode & 0x80)	// Make Or Break Code ?
		goto	BreakCode;
#if 0	
	// Ctrl	+ Alt + Del
	if( IS_L_CTRL_DOWN(key) && IS_L_ALT_DOWN(key))
	{
		if(E0Flag == 1 && ScanCode == MC_DELETE)
		{
			extern	void reboot();
			printf("Reboot System Now ...\n");
			int i = 100000;
			while(i--);
			reboot();
		}
		if(ScanCode ==  MC_BACKSPACE)
		{
			extern	void poweroff();
			printf("Shutdown System Now ...\n");
			int i = 100000;
			while(i--);
			poweroff();
		}
	}
#endif

	switch(ScanCode)
	{
	case	MC_L_SHIFT:	SET_L_SHIFT_DOWN(key);	goto End;
	case	MC_R_SHIFT:	SET_R_SHIFT_DOWN(key);	goto End;

	case	MC_CTRL:
		E0Flag?SET_R_CTRL_DOWN(key):SET_L_CTRL_DOWN(key);
		goto End;
	case	MC_ALT:

		E0Flag?SET_R_ALT_DOWN(key):SET_L_ALT_DOWN(key);
		goto End;
	}
		
	goto	End;

BreakCode:
	switch(ScanCode)
	{
	case	BC_L_SHIFT:	SET_L_SHIFT_UP(key);	goto End;
	case	BC_R_SHIFT:	SET_R_SHIFT_UP(key);	goto End;
	case	BC_CTRL:
		E0Flag?SET_R_CTRL_UP(key):SET_L_CTRL_UP(key);
		goto End;
	case	BC_ALT:
		E0Flag?SET_R_ALT_UP(key):SET_L_ALT_UP(key);
		goto End;
	}

End:

	key &= 0xFFFFFF00;
	key |= (((E0Flag)?0x00:ScanCode) & 0xFF);

	E0Flag = 0;

	return key;

}



char ParseKbdInput(int k)
{
	//unsigned int k;
	unsigned int inx;
	unsigned char chr;
	static unsigned char CapsLock;

	k = _parse_kbd(k);

	k = k & 0xFF;

	if(k == -1) return ;

	inx = k & 0xFF;
	chr = 0;

	if(inx >= sizeof(kbdCharTable)) goto End;

	// CapsLock ON OR OFF
	if(inx == MC_CAPSLOCK)
		CapsLock = (CapsLock)?0:1;


	//if((!IS_L_SHIFT_DOWN(k) && !IS_R_SHIFT_DOWN(k) && CapsLock == 0)
	//|| ((IS_L_SHIFT_DOWN(k) || IS_R_SHIFT_DOWN(k)) && CapsLock ==1))
	if(!IS_L_SHIFT_DOWN(k) && !IS_R_SHIFT_DOWN(k))
	{
		chr = kbdCharTable[inx];
		if(CapsLock ==1 && chr >= 'a' && chr <= 'z')
			chr -= 32;
	}
	else
	{
		chr = kbdShiftCharTable[inx];
		if(CapsLock ==1 && chr >= 'A' && chr <= 'Z')
			chr += 32;
	}

End:
	if(chr != 0)
	{
		printf("%c", chr);
		return (char)chr;
	}

	return -1;
}

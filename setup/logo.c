#include<string.h>
extern const char *version;
extern void printString(const char *s, unsigned char color);
void show_logo()
{
#if 1
int i;
#define	VER_SIZE	81
char ver[VER_SIZE];
memset(ver, ' ', VER_SIZE);
ver[VER_SIZE-1] = 0;
//printString(ver, 0x70);
#if 0
printString(
"                     ** **       *******   **    ** **                          "
"                    *** **       ********  **    ** ***                         "
"                   **** **       **     ** **    ** ****                        "
"                  ** ** **       *******   ******** ** **                       "
"                 **  ** **       ******    ******** **  **                      "
"                ******* **       **        **    ** *******                     "
"               **    ** ******** **        **    ** **    **                    "
"              **     ** ******** **        **    ** **     **                   "
, 0x9F);
#endif

memset(ver, ' ', VER_SIZE);
strcpy(ver, version);
for(i=0; i<VER_SIZE; i++)
	if(ver[i] == '\0')
	{
		ver[i] = ' ';
		break;
	}
ver[VER_SIZE-1] = 0;
printString(ver, 0x70);
#endif
#if 0
for(i=1; i<80*1*2; i+=2)
	*((unsigned char *)(0xB8000+i)) = 0x00;
for(i=1+80*1*2; i<80*9*2; i+=2)
	*((unsigned char *)(0xB8000+i)) = 0x1F;
for(i=1+80*9*2; i<80*10*2; i+=2)
	*((unsigned char *)(0xB8000+i)) = 0x7F;
#endif
}

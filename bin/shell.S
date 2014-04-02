/*
 *--------------------------------------------------------------------------
 *   File Name:	shell.S
 * 
 *      Author:	Zhao Yanbai [zhaoyanbai@126.com]
 * 			Wed Feb 24 17:47:27 2010
 * 
 * Description:	none
 * 
 *--------------------------------------------------------------------------
 */
.global	_start
_start:
	nop
	nop
	nop
loop:
	#jmp	loop
	call	shell
	nop
	nop
	nop
	nop
	nop
	nop
	nop
	nop
die:
	jmp	die

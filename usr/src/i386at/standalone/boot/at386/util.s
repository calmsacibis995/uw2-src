/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

/	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

/
/	 Copyrighted as an unpublished work.
/	 (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
/	 All rights reserved.
/	
/	 RESTRICTED RIGHTS
/	
/	 These programs are supplied under a license.  They may be used,
/	 disclosed, and/or copied only as permitted under such license
/	 agreement.  Any copy must contain the above copyright notice and
/	 this restricted rights notice.  Use, copying, and/or disclosure
/	 of the programs is strictly prohibited unless otherwise provided
/	 in the license agreement.

	.file	"util.s"

.ident	"@(#)stand:i386at/standalone/boot/at386/util.s	1.2.1.3"
	.ident  "$Header: $"

/	protected mode putchar; character on stack.

	.globl	bputchar
bputchar:
	pushl	%ebp			/ save registers
	pushl	%ebx
	pushl	%edi
	pushl	%esi

	call	goreal

	xor	%ebx, %ebx		/ use page zero (%bh)
	movb	$1, %bl
	addr16
	movb	0x14(%esp), %al		
	movb	$14, %ah		/ teletype putchar
	int	$0x10			/ issue request

	data16
	call	goprot

	popl	%esi			/ restore registers
	popl	%edi
	popl	%ebx
	popl	%ebp

	ret

/	----------------------------------------------------
/ Return TRUE if a character is waiting to be read.
/
	.globl	ischar

ischar:
	pushl	%ebp			/ save registers
	pushl	%ebx
	pushl	%edi
	pushl	%esi

	call	goreal

	data16
	xorl	%edx, %edx		/ clear %edx for result

	movb	$1, %ah			/ setup for bios test for a char
	int	$0x16			/ sets the zero flag if char is waiting

	jz	nochar			/ no char waiting

	data16
	movl	$1, %edx		/ char waiting: return TRUE

nochar:
	data16
	call	goprot	

	popl	%esi			/ restore registers
	popl	%edi
	popl	%ebx
	popl	%ebp

	movl	%edx, %eax		/ set up return; goprot trashes %eax

	ret


/	--------------------------------------------
/ 	Call BIOS wait routine to wait for 1 second; programming the interval
/	timer directly does not seem to be reliable.
/ 	- decreased resolution to cut down on overhead of mode switching
/

	.globl	wait1s
wait1s:
	push	%ebp			/ save registers
	push	%edi
	push	%esi
	push	%ebx

	call	goreal

	data16
	movl	$0x0f, %ecx
	data16
	movl	$0x4240, %edx
	movb	$0x86, %ah		/ setup for bios wait
	int	$0x15			/ BIOS utility function

	data16
	call	goprot

	pop	%ebx			/ restore registers
	pop	%esi
	pop	%edi
	pop	%ebp

	ret


/	--------------------------------------------
/	memcpy(dest, src, cnt): works in exactly the same fashion as the 
/	libc memcpy; addresses are physaddr's.
	.globl	bmemcpy
bmemcpy:
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	movl	16(%esp), %edi	/ %edi = dest address
	movl	20(%esp), %esi	/ %esi = source address
	movl	24(%esp), %ecx	/ %ecx = length of string
	movl	%edi, %eax	/ return value from the call

	movl	%ecx, %ebx	/ %ebx = number of bytes to move
	shrl	$2, %ecx	/ %ecx = number of words to move
	rep ; smovl		/ move the words

	movl	%ebx, %ecx	/ %ecx = number of bytes to move
	andl	$0x3, %ecx	/ %ecx = number of bytes left to move
	rep ; smovb		/ move the bytes

	popl	%ebx
	popl	%esi
	popl	%edi
	ret

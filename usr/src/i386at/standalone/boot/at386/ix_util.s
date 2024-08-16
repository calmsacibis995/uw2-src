/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
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

	.file	"ix_util.s"

.ident	"@(#)stand:i386at/standalone/boot/at386/ix_util.s	1.3.1.4"
	.ident	"$Header: $"

	.text

/ Do a BIOS int call.  Called by doint(&param)
/
/		where: param -> struct int_pb
/struct int_pb {
/	ushort	intval, 	/* 0x0 */
/		ax, bx,		/* 0x2 0x4 */
/		cx, dx, 	/* 0x6 0x8 */
/		bp, es,		/* 0xa 0xc */
/		si,		/* 0xe  (returned only) */
/		ds; };		/* 0x10 */
	.globl	doint
doint:
	pushl	%ebp	/save non-volatile registers
	pushl	%ebx
	pushl	%edi
	pushl	%esi

	movl	0x14(%esp), %eax
	movw	(%eax), %ax	/ pick up the int code to use
	movb	%al, intcode+1

	call	goreal		/ get real

	sti
/ 	set up registers for the call
	addr16
	data16
	mov	0x14(%esp),%edi / move int_pb fields to registers
	addr16
	mov	4(%edi), %ebx
	addr16
	mov	6(%edi), %ecx
	addr16
	mov	8(%edi), %edx
	addr16
	mov	10(%edi), %ebp
	addr16
	movw	12(%edi), %es
	addr16
	mov	14(%edi), %esi
/	save real mode segment register
	push	%ds
	addr16
	push	2(%edi)		/ push ax because of %ds change
	addr16
	mov	16(%edi), %eax
	movw	%ax,%ds
	clc			/ clear CARRY to see BIOS call effect
	pop	%eax

intcode:
	int	$0		/ It is self modified

	cli
	cld
/	restore real mode segment register
	pop	%ds

	pushf			/ save carry for later
	push	%eax		/ and stash the returned registers
	push	%ebx		/ that are going to get trashed
	push	%es		/ by goprot call

	data16
	call	goprot		/ protect mode

	movl	0x1C(%esp), %edi
	popw	%ax		/ pop the saved registers (shorts)
	movw	%ax, 12(%edi)
	popw	%ax		/ transferring them to the int call param block
	movw	%ax, 4(%edi)
	popw	%ax
	movw	%ax, 2(%edi)
	movw	%cx, 6(%edi)
	movw	%dx, 8(%edi)
	movw	%bp, 10(%edi)
        xorl    %eax, %eax      / initalize return to zero
	popfw                   / get carry
	jnc     dixt
	incl    %eax            / return != 0 for carry set
dixt:
	popl    %esi            / restore registers
	popl    %edi
	popl    %ebx
	popl    %ebp
	ret


/	longjmp(env, val)
/ will generate a "return(val)" from
/ the last call to
/	setjmp(env)
/ by restoring registers ip, sp, bp, bx, si, and di from 'env'
/ and doing a return.

/ entry    reg	offset from (%si)
/ env[0] = %ebx	 0	/ register variables
/ env[1] = %esi	 4
/ env[2] = %edi	 8
/ env[3] = %ebp	 12	/ stack frame
/ env[4] = %esp	 16
/ env[5] = %eip	 20

	.globl	setjmp
	.align	4

setjmp:
	movl	4(%esp),%eax	/ jmpbuf address
	movl	%ebx,0(%eax)	/ save ebx
	movl	%esi,4(%eax)	/ save esi
	movl	%edi,8(%eax)	/ save edi
	movl	%ebp,12(%eax)	/ save caller's ebp
	popl	%edx		/ return address
	movl	%esp,16(%eax)	/ save caller's esp
	movl	%edx,20(%eax)
	subl	%eax,%eax	/ return 0
	jmp	*%edx

	.globl	longjmp
	.align	4
longjmp:
	cld			/ in case reps are going in reverse
	movl	4(%esp),%edx	/ first parameter after return addr
	movl	8(%esp),%eax	/ second parameter
	movl	0(%edx),%ebx	/ restore ebx
	movl	4(%edx),%esi	/ restore esi
	movl	8(%edx),%edi	/ restore edi
	movl	12(%edx),%ebp	/ restore caller's ebp
	movl	16(%edx),%esp	/ restore caller's esp
	test	%eax,%eax	/ if val != 0
	jne	.ret		/ 	return val
	incl	%eax		/ else return 1
.ret:
	jmp	*20(%edx)	/ return to caller

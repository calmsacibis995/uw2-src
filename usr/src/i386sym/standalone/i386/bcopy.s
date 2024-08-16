/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

/
/ Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990
/ Sequent Computer Systems, Inc.   All rights reserved.
/
/ This software is furnished under a license and may be used
/ only in accordance with the terms of that license and with the
/ inclusion of the above copyright notice.   This software may not
/ be provided or otherwise made available to, or used by, any
/ other person.  No title to or ownership of the software is
/ hereby transferred.
/

		.ident	"@(#)stand:i386sym/standalone/i386/bcopy.s	1.1"

include(asm.m4)

/
/ bcopy(from,to,count)
/	char *from,*to;
/	unsigned count;
/
/ Fast byte machine copy.  Here because libc version is bigger (and smarter).
/ Plus,we want real move strings (not a work-around that may be in libc)
/ Does not handle overlap so be careful
/
/
	.text
	.globl	bcopy
	.align	4
bcopy:
	movl	%edi,%eax		/ save registers
	movl	%esi,%edx
	movl	SPARG0,%esi		/ source 
	movl	SPARG1,%edi		/ destination 
	movl	SPARG2,%ecx		/ count 
	shrl	$2,%ecx			/ number of double words to move 
	rep;	smovl			/ move double words (NOP if zero) 
	movb	SPARG2,%cl		/ low order byte of count 
	andb	$3,%cl			/ remaining number to transfer 
	rep;	smovb			/ move bytes (NOP if zero) 
	movl	%edx,%esi		/ restore registers 
	movl	%eax,%edi
	RETURN

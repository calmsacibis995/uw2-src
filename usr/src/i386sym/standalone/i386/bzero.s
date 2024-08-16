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

		.ident	"@(#)stand:i386sym/standalone/i386/bzero.s	1.1"

include(asm.m4)

/
/ bzero(base,count)
/	char *base;
/	unsigned count;
/
/ Fast byte zero.  Smaller than libc's version.
/
/
	.text
	.globl	bzero
	.align	4
bzero:
	movl	%edi,%edx		/* save register */
	movl	SPARG0,%edi		/* base */
	movl	SPARG1,%ecx		/* length */
	shrl	$2,%ecx			/* number of longs */
	xorl	%eax,%eax		/* write zeros */
	rep;	sstol			/* clear double words */
	movb	SPARG1,%cl		/* low byte of length */
	andb	$3,%cl			/* number of bytes left */
	rep;	sstob			/* clear remaining bytes */
	movl	%edx,%edi		/* restore register */
	RETURN

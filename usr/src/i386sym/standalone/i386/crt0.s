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

		.ident	"@(#)stand:i386sym/standalone/i386/crt0.s	1.1"

include(asm.m4)

/
/ start()
/
/ Entry point for standalone programs.  Standalone runs
/ in a flat 4 Gig segment with interrupts disabled.
/ Prior to starting up the program it must have a stack 
/ initialized, the SLIC initialized, and its BSS cleared.
/
	.text
	.globl	start
	.align	4
start:
	movl	CD_LOC+c_bottom,%esp	/* set initial stack pointer */
	CALL	slic_init		/* set up slic stuff */
.L1:	CALL	clearbss		/* clear our own bss */
	CALL	main			/* do the work */
	pushl	%eax			/* return value */
.L2:	CALL	exit			/* exit */
	jmp	.L2			/* should not happen */

	/* relative speed of machine */
	.data
	.align	4
	.globl	cpuspeed
cpuspeed:
	.long	8			/* XXX is this really needed? */

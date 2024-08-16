/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:util/lsub.s	1.3"
	.file	"util/lsub.s"

include(../svc/asm.m4)

/
/ dl_t
/ lsub(dl_t, dl_t)
/	This function returns the 64-bit result of subtracting
/	the two 64-bit arguments.
/
/ Calling State/Exit State:
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.
/
/ Remarks:
/	The return value is an eight-byte structure; the C calling
/	convention is that the pointer to the structure is passed
/	both in %eax and as an implied first argument (i.e., SPARG0).
/	This routine uses the pointer in %eax, and ignores the implied
/	argument.
/
ENTRY(lsub)
	movl	SPARG1, %edx		/* left operand low */
	movl	SPARG3, %ecx		/* right operand low */
	subl	%ecx, %edx
	movl	SPARG2, %ecx		/* left operand high */
	movl	%edx, (%eax)		/* return low word */
	movl	SPARG4, %edx		/* right operand high */
	sbbl	%edx, %ecx		/* subtract high with borrow */
	movl	%ecx, 4(%eax)		/* return high word */
	ret	$4			/* clear structure pointer and ret */
	.size	lsub,.-lsub

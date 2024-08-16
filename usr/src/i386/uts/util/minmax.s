/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:util/minmax.s	1.2"

include(../svc/asm.m4)
include(assym_include)


/
/ int
/ min(uint, uint)
/	Returns the unsigned integer min of two arguments.
/
/ Calling State/Exit State:
/	None.
/
ENTRY(min)
	movl	SPARG0, %eax
	movl	SPARG1, %ecx
	cmpl	%ecx, %eax
	jbe	.minxit			/ NOTE: unsigned comparison/branch
	movl	%ecx, %eax
.minxit:
	ret
	SIZE(min)

/
/ int
/ max(uint, uint)
/	Returns the unsigned integer max of two arguments.
/
/ Calling State/Exit State:
/	None.
/
ENTRY(max)
	movl	SPARG0, %eax
	movl	SPARG1, %ecx
	cmpl	%ecx, %eax
	jae	.maxit			/ NOTE: unsigned comparison/branch
	movl	%ecx, %eax
.maxit:
	ret
	SIZE(max)

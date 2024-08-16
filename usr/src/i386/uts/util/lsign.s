/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:util/lsign.s	1.2"
        .file   "util/lsign.s"

include(../svc/asm.m4)

/
/ int
/ lsign(dl_t)
/	This function returns the value of the sign bit of
/	the 64-bit argument (assumes POSITIVE == 0, NEGATIVE == 1).
/
/ Calling State/Exit State:
/	This function assumes caller has provided sufficient locking
/	on the arguments prior to the call.
/
ENTRY(lsign)
	movl	SPARG1, %eax
	roll	%eax
	andl	$1, %eax
	ret
	SIZE(lsign)

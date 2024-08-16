/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.


/	Copyright (c) 1987, 1988 Microsoft Corporation
/	  All Rights Reserved

/	This Module contains Proprietary Information of Microsoft
/	Corporation and should be treated as Confidential.

	.file	"execseg.s"

	.ident	"@(#)xcplibx:i386/xcplib/libx/sys/execseg.s	1.2"
	.ident  "$Header: execseg.s 1.1 91/07/04 $"


	.globl	execseg
	.globl	_cerror

execseg:
	MCOUNT			/ subroutine entry counter if profiling
	movl	$EXECSEG,%eax
	lcall	SYSCALL
	jc	_cerror
	ret

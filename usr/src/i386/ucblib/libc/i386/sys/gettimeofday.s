/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.


.ident	"@(#)ucb:i386/ucblib/libc/i386/sys/gettimeofday.s	1.1"
/ gettimeofday


	.file	"gettimeofday.s"

	.text

	.globl  _cerror
	.weak	_abi_gettimeofday

/ _fwdef_(`gettimeofday'):
/ _abi_gettimeofday:
/	MCOUNT
	movl	$GETTIMEOFDAY,%eax
	lcall	$0x7,$0
	jc 	_cerror
	xorl	%eax,%eax
	ret

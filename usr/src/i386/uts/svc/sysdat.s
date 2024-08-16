/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:svc/sysdat.s	1.1"
	.ident	"$Header: $"
	.file	"svc/sysdat.s"
 

include(../svc/asm.m4)
include(assym_include)

	.set hrestime, _A_KVSYSDAT
	.globl hrestime
	.type hrestime, "object"
	.size hrestime, _A_TIMESTR_SIZ

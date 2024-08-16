/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386sym:svc/syms_p.s	1.2"
	.ident	"$Header: $"

/ Platform-specific symbol definitions; concatenated onto syms.s

	SYM_DEF(KVSLIC_LMASK, _A_KVSLIC+_A_SL_LMASK)
	SYM_DEF(KVSLIC_ICTL, _A_KVSLIC+_A_SL_ICTL)
	SYM_DEF(KVPLOCAL_L_SLIC_DELAY, _A_KVPLOCAL+_A_L_SLIC_DELAY)
	SYM_DEF(KVPLOCAL_L_SLIC_LONG_DELAY, _A_KVPLOCAL+_A_L_SLIC_LONG_DELAY)
	SYM_DEF(kvetc, _A_KVETC)

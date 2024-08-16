/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern:io/tp/tpath_stub.s	1.2"
	.ident	"$Header: $"
/
/ This file contains the stubs for tpath.
/
include(../../util/mod/stub.m4)

MODULE(tpath, STUB_UNLOADABLE)
USTUB(tpath, tp_disconnect, mod_zero, 1)
USTUB(tpath, tp_getmajor, mod_zero, 1)
END_MODULE(tpath)

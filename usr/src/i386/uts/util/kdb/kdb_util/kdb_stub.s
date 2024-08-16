/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:util/kdb/kdb_util/kdb_stub.s	1.2"
	.ident	"$Header: $"
/
/ This file contains the stubs for kdb_util.
/
include(../../../util/mod/stub.m4)

MODULE(kdb_util, STUB_LOADONLY)
WSTUB(kdb_util, kdb_printf, mod_zero)
WSTUB(kdb_util, kdb_check_aborted, mod_zero)
END_MODULE(kdb_util)

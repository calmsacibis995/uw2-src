/	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

/
/ This file contains the stubs for ccnv.
/
	.ident	"@(#)kern-i386:util/ccnv/ccnv_stub.s	1.3"
include(../../util/mod/stub.m4)

MODULE(ccnv, STUB_UNLOADABLE)
USTUB(ccnv, ccnv_unix2dos, mod_enoload,3)
USTUB(ccnv, ccnv_dos2unix, mod_enoload,3)
USTUB(ccnv, dos2unixfn, mod_enoload,3)
USTUB(ccnv, unix2dosfn, mod_enoload,3)
END_MODULE(ccnv)

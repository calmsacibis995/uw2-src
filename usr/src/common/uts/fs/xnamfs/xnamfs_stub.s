/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern:fs/xnamfs/xnamfs_stub.s	1.2"
	.ident	"$Header: $"
/
/ This file contains the stubs for xnamfs.
/
include(../../util/mod/stub.m4)

MODULE(xnamfs, STUB_LOADONLY)
STUB(xnamfs, xnamvp, mod_zero)
STUB(xnamfs, xnampreval, mod_enoload)
STUB(xnamfs, sdget, mod_enoload)
STUB(xnamfs, creatsem, mod_enoload)
WSTUB(xnamfs, xsemfork, mod_zero)
WSTUB(xnamfs, xsdexit, mod_einval)
WSTUB(xnamfs, xsd_cleanup, mod_einval)
WSTUB(xnamfs, xsdfork, mod_zero)
WSTUB(xnamfs, xsdfree, mod_einval)
WSTUB(xnamfs, sdenter, mod_einval)
WSTUB(xnamfs, sdleave, mod_einval)
WSTUB(xnamfs, sdgetv, mod_einval)
WSTUB(xnamfs, sdwaitv, mod_einval)
WSTUB(xnamfs, opensem, mod_einval)
WSTUB(xnamfs, sigsem, mod_einval)
WSTUB(xnamfs, waitsem, mod_einval)
WSTUB(xnamfs, nbwaitsem, mod_einval)
WSTUB(xnamfs, closesem, mod_einval)
END_MODULE(xnamfs)

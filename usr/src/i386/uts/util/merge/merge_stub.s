/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386:util/merge/merge_stub.s	1.2"
	.ident	"$Header: $"

/
/ This file contains unloadable Merge 386 stubs.
/
include(../../util/mod/stub.m4)

.mod_one:
	movl	$1, %eax
	ret

MODULE(merge386, STUB_UNLOADABLE)
WSTUB(merge386, portalloc, .mod_one)
WSTUB(merge386, portfree, .mod_one)
WSTUB(merge386, floppy_free, .mod_one)

WSTUB(merge386, isdosexec, mod_zero)
WSTUB(merge386, asy_is_assigned, mod_zero)
WSTUB(merge386, com_ppiioctl, mod_zero)		/Old style.
WSTUB(merge386, com_ppi_ioctl, mod_zero)	/Change from com_ppiioctl.
WSTUB(merge386, com_ppi_strioctl, mod_zero)	/New
WSTUB(merge386, flp_for_dos, mod_zero)
WSTUB(merge386, kdppi_ioctl, mod_zero)
WSTUB(merge386, mrg_in_use, mod_zero)
END_MODULE(merge386)

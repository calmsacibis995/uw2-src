/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386at:io/hba/mcst/mcst_stub.s	1.3"
	.ident	"$Header: $"

/
/ This file contains the DLM stubs for mcst.
/

include(../../../util/mod/stub.m4)

MODULE(mcst, STUB_LOADONLY)
STUB(mcst, mcst_bdinit, mod_zero)
WSTUB(mcst, mcst_drvinit, mod_zero)
WSTUB(mcst, mcst_cmd, mod_enoload)
WSTUB(mcst, mcst_int, mod_zero)
END_MODULE(mcst)

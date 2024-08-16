/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-i386at:io/hba/mcesdi/mcesdi_stub.s	1.3"
	.ident	"$Header: $"

/
/ This file contains the DLM stubs for mcesdi.
/

include(../../../util/mod/stub.m4)

MODULE(mcesdi, STUB_LOADONLY)
STUB(mcesdi, mces_bdinit, mod_zero)
WSTUB(mcesdi, mces_drvinit, mod_zero)
WSTUB(mcesdi, mces_cmd, mod_enoload)
WSTUB(mcesdi, mces_int, mod_zero)
END_MODULE(mcesdi)

/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

/	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF
/	UNIX System Laboratories, Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern-pdi:io/hba/athd/athd_stub.s	1.2"
	.ident	"$Header: $"
/
/ This file contains the DLM stubs for athd.
/
include(../../../util/mod/stub.m4)

MODULE(athd, STUB_LOADONLY)
STUB(athd, athd_bdinit, mod_zero)
WSTUB(athd, athd_drvinit, mod_zero)
WSTUB(athd, athd_cmd, mod_enoload)
WSTUB(athd, athd_int, mod_zero)
END_MODULE(athd)

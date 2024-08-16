/	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.
/	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.
/	  All Rights Reserved

/	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.
/	The copyright notice above does not evidence any
/	actual or intended publication of such source code.

	.ident	"@(#)kern:proc/ipc/ipc_stub.s	1.2"
	.ident	"$Header: $"
/
/ This file contains the stubs for ipc.
/
include(../../util/mod/stub.m4)

MODULE(ipc, STUB_LOADONLY)
STUB(ipc, shmsys, nosys)
STUB(ipc, semsys, nosys)
STUB(ipc, msgsys, nosys)
STUB(ipc, shmconv, nosys)
STUB(ipc, semconv, nosys)
STUB(ipc, msgconv, nosys)
STUB(ipc, shmfork, mod_zero)
STUB(ipc, shmexec, mod_zero)
STUB(ipc, ipcaccess, nosys)
WSTUB(ipc, shmexit, mod_zero)
WSTUB(ipc, semexit, mod_zero)
END_MODULE(ipc)

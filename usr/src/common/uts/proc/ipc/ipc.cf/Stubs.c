/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/ipc/ipc.cf/Stubs.c	1.2"
#ident	"$Header: $"

int shmconv() { return nosys(); }
int shmsys() { return nosys(); }
void shmexec() {}
void shmexit() {}
void shmfork() {}

int semconv() { return nosys(); }
int semsys() { return nosys(); }
void semexit() {}

int msgconv() { return nosys(); }
int msgsys() { return nosys(); }

int ipcaccess() { return nosys(); }
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:sys/msgsys.c	1.6.3.1"
#ifdef __STDC__
	#pragma weak msgctl = _msgctl
	#pragma weak msgget = _msgget
	#pragma weak msgrcv = _msgrcv
	#pragma weak msgsnd = _msgsnd
#endif
#include	"synonyms.h"
#include	"sys/types.h"
#include	"sys/ipc.h"
#include	"sys/msg.h"

#define	MSGSYS	49

#define	MSGGET	0
#define	MSGCTL	1
#define	MSGRCV	2
#define	MSGSND	3

extern long syscall();

int
msgget(key, msgflg)
key_t key;
int msgflg;
{
	return(syscall(MSGSYS, MSGGET, key, msgflg));
}

int
msgctl(msqid, cmd, buf)
int msqid, cmd;
struct msqid_ds *buf;
{
	return(syscall(MSGSYS, MSGCTL, msqid, cmd, buf));
}

int
msgrcv(msqid, msgp, msgsz, msgtyp, msgflg)
int msqid;
void *msgp;
size_t msgsz;
long msgtyp;
int msgflg;
{
	return(syscall(MSGSYS, MSGRCV, msqid, msgp, msgsz, msgtyp, msgflg));
}

int
msgsnd(msqid, msgp, msgsz, msgflg)
int msqid;
const void *msgp;
size_t msgsz;
int msgflg;
{
	return(syscall(MSGSYS, MSGSND, msqid, msgp, msgsz, msgflg));
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:sys/shmsys.c	1.7.3.5"
#ifdef __STDC__
	#pragma weak shmat = _shmat
	#pragma weak shmctl = _shmctl
	#pragma weak shmdt = _shmdt
	#pragma weak shmget = _shmget
#endif
#include	"synonyms.h"
#include	"sys/types.h"
#include	"sys/ipc.h"
#include	"sys/shm.h"

#define	SHMSYS	52

#define	SHMAT	0
#define	SHMCTL	1
#define	SHMDT	2
#define	SHMGET	3

extern long syscall();

VOID *
#ifdef __STDC__
shmat(int shmid, const VOID *shmaddr, int shmflg)
#else
shmat(shmid, shmaddr, shmflg)
int shmid;
const VOID *shmaddr;
int shmflg;
#endif
{
	return((char *)syscall(SHMSYS, SHMAT, shmid, shmaddr, shmflg));
}

int
#ifdef __STDC__
shmctl(int shmid, int cmd, struct shmid_ds *buf)
#else
shmctl(shmid, cmd, buf)
int shmid, cmd;
struct shmid_ds *buf;
#endif
{
	return(syscall(SHMSYS, SHMCTL, shmid, cmd, buf));
}

int
#ifdef __STDC__
shmdt(const VOID *shmaddr)
#else
shmdt(shmaddr)
char *shmaddr;
#endif
{
	return(syscall(SHMSYS, SHMDT, shmaddr));
}

int
#ifdef __STDC__
shmget(key_t key, size_t size, int shmflg)
#else
shmget(key, size, shmflg)
key_t key;
size_t size;
int shmflg;
#endif
{
	return(syscall(SHMSYS, SHMGET, key, size, shmflg));
}

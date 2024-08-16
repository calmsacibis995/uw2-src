/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:sys/semsys.c	1.6.3.2"
#ifdef __STDC__
	#pragma weak semctl = _semctl
	#pragma weak semget = _semget
	#pragma weak semop = _semop
#endif
#include	"synonyms.h"
#include	"sys/types.h"
#include	"sys/ipc.h"
#include	"sys/sem.h"
#ifdef __STDC__
#include	<stdarg.h>
#else
#include	<varargs.h>
#endif

#define	SEMSYS	53

#define	SEMCTL	0
#define	SEMGET	1
#define	SEMOP	2

extern long syscall();

union semun
{
	int		val;
	struct semid_ds	*buf;
	ushort_t	*array;
};

int
#ifdef __STDC__
semctl(int id, int num, int cmd, ...)
#else
semctl(va_alist)va_dcl
#endif
{
	int val, retry;
	va_list ap;
#ifdef __STDC__
	va_start(ap, cmd);
#else
	int id, num, cmd;

	va_start(ap);
	id = va_arg(ap, int);
	num = va_arg(ap, int);
	cmd = va_arg(ap, int);
#endif
	/*
	* Due to an unfortunate documentation miscue, the intended use of
	* semctl() changed from an optional 4th argument with varying
	* scalar types to a union--one that wasn't declared in any header!
	* For most existing implementations, this wasn't a real problem
	* since a union of int and a couple of pointers was passed the
	* same as a simple int or pointer.  However, other implementations
	* also have to be handled, and there is no "right" answer, given
	* that some code might exist (other than test suites) that passes
	* the 4th argument as a true union.
	*
	* This code takes an intermediate position.  Since taking the 4th
	* argument as a simple scalar is less likely to cause faults at
	* user level, we try that first.  The kernel won't dereference a
	* bad pointer value.  The hope is that if we guessed wrong, the
	* system call will fail, and we give it another try as if a union
	* was passed, but only for those cmd values that need a 4th arg.
	*/
	retry = 1;
	switch (cmd)
	{
	default:
		val = 0;
		retry = 0;
		break;
	case SETVAL:
		val = va_arg(ap, int);
		break;
	case GETALL:
	case SETALL:
		val = (int)va_arg(ap, ushort_t *);
		break;
	case 1: /*IPC_O_SET*/
	case 2: /*IPC_O_STAT*/
	case IPC_STAT:
	case IPC_SET:
		val = (int)va_arg(ap, struct semid_ds *);
		break;
	}
	while ((val = syscall(SEMSYS, SEMCTL, id, num, cmd, val)) == -1
		&& retry)
	{
		va_end(ap);
		retry = 0;
#ifdef __STDC__
		va_start(ap, cmd);
#else
		va_start(ap);
		(void)va_arg(ap, int);
		(void)va_arg(ap, int);
		(void)va_arg(ap, int);
#endif
		val = va_arg(ap, union semun).val;
	}
	va_end(ap);
	return val;
}

int
semget(key, nsems, semflg)
key_t key;
int nsems, semflg;
{
	return(syscall(SEMSYS, SEMGET, key, nsems, semflg));
}

int
semop(semid, sops, nsops)
int semid;
struct sembuf *sops;
unsigned int nsops;
{
	return(syscall(SEMSYS, SEMOP, semid, sops, nsops));
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_LOCK_H	/* wrapper symbol for kernel use */
#define _MEM_LOCK_H	/* subject to change without notice */

#ident	"@(#)kern:mem/lock.h	1.9"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * flags for locking procs and texts: stored in p_plock (see proc.h)
 */
#define	PROCLOCK 1
#define	TXTLOCK	 2
#define	DATLOCK	 4

#if defined(_KERNEL)

#define	MEMLOCK	 8

int textlock(void);
int datalock(void);
int proclock(void);
void tunlock(void);
void dunlock(void);
void punlock(void);

#else

#define	UNLOCK	 0

#ifdef __STDC__
int plock(int);
#else
int plock();
#endif /* __STDC__ */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_LOCK_H */

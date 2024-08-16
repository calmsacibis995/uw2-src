/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STDLOCK_H
#define _STDLOCK_H
#ident	"@(#)libmalloc:common/stdlock.h	1.1"
/*
* stdlock.h - locking internal to libc.
*/

#include <sys/types.h>	/* for id_t */

typedef struct
{
	int	lock[2];	/* [0] is nonzero if locked; [1] is count */
} StdLock;

#ifdef _REENTRANT

#define STDLOCK(p)	_stdlock(p)
#define STDUNLOCK(p)	_stdunlock(p)
#define STDTRYLOCK(p)	_stdtrylock(p)

#ifdef __STDC__
	extern void (*_libc_block)(int *);
	extern void (*_libc_unblock)(int *);
	extern id_t (*_libc_self)(void);

	extern void _stdlock(StdLock *);
	extern void _stdunlock(StdLock *);
	extern int _stdtrylock(StdLock *);
#else
	extern void (*_libc_block)();
	extern void (*_libc_unblock)();
	extern id_t (*_libc_self)();

	extern void _stdlock();
	extern void _stdunlock();
	extern int _stdtrylock();
#endif

#else /*!_REENTRANT*/

#define STDLOCK(p)
#define STDUNLOCK(p)
#define STDTRYLOCK(p)	0	/* success */

#endif /*_REENTRANT*/

#endif /*_STDLOCK_H*/

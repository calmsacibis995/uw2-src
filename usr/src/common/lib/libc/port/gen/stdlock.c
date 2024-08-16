/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/stdlock.c	1.4"
/*LINTLIBRARY*/

#include "synonyms.h"
#include "stdlock.h"

#ifdef _REENTRANT

void
#ifdef __STDC__
_stdlock(StdLock *slp)	/* portable exclusion lock (at least a try) */
#else
_stdlock(slp)StdLock *slp;
#endif
{
	/*
	* There's a timing window between the test and the set.
	* Sorry.  There's no portable way to do this.
	*/
	if (slp->lock[0] != 0)
		(*_libc_block)(&slp->lock[0]);
	else
		slp->lock[0] = 1;	/* lock it */
}

void
#ifdef __STDC__
_stdunlock(register StdLock *slp)	/* portable unlock (at least a try) */
#else
_stdunlock(slp)register StdLock *slp;
#endif
{
	slp->lock[0] = 0;	/* unlock it */
	if (slp->lock[1] != 0)
		(*_libc_unblock)(&slp->lock[0]);
}

int
#ifdef __STDC__
_stdtrylock(StdLock *slp)	/* nonblocking portable exclusion lock (a try) */
#else
_stdtrylock(slp)StdLock *slp;
#endif
{
	/*
	* There's a timing window between the test and the set.
	* Sorry.  There's no portable way to do this.
	*/
	if (slp->lock[0] == 0)
	{
		slp->lock[0] = 1;	/* lock it */
		return 0;
	}
	return 1;
}

void
#ifdef __STDC__
_stdtryunlock(register StdLock *slp)	/* portable unlock to match (a try) */
#else
_stdtryunlock(slp)register StdLock *slp;
#endif
{
	slp->lock[0] = 0;
}

static void
#ifdef __STDC__
block_unblock(int *p)	/* should only be reached if locking error occurs */
#else
block_unblock(p)int *p;
#endif
{
	p[0] = 0;
	p[1] = 0;
}

static id_t
#ifdef __STDC__
myself(void)
#else
myself()
#endif
{
	return ~(id_t)0;	/* as good as any other nonzero answer */
}

#ifdef __STDC__
void (*_libc_block)(int *) = &block_unblock;
void (*_libc_unblock)(int *) = &block_unblock;
id_t (*_libc_self)(void) = &myself;
#else
void (*_libc_block)() = &block_unblock;
void (*_libc_unblock)() = &block_unblock;
id_t (*_libc_self)() = &myself;
#endif

#endif /*_REENTRANT*/

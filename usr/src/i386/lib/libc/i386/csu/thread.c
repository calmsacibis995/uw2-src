/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:csu/thread.c	1.6"
/*
 * thread initialization and errno retrieval.  THIS FILE MUST BE STATICALLY
 * LINKED WITH THE APPLICATION.  Here we define an lwp descriptor, used
 * to implement lwp_self in single-threaded programs.  In multithreaded
 * programs, libthread substitutes another lwp descriptor and provides a
 * thread descriptor.
 */
#ifdef __STDC__
#pragma weak __errno = __thr_errno
#endif

#include <thread.h>

extern void *__lwp_private(void *);

__lwp_desc_t **__lwp_priv_datap;	/* lwp-private data area */
__lwp_desc_t __lwp_first;		/* descriptor for first lwp */

int	__multithreaded;		/* Set by libthread startup. */

/*
 * __thr_init()
 *	Initialize the lwp-private data area pointer and such.
 *
 */
void
__thr_init(void)
{
	__lwp_priv_datap = (__lwp_desc_t **)__lwp_private((void *)&__lwp_first);
}

/*
 * __thr_errno()
 *	Return a pointer to this thread's private errno.
 */
int *
__thr_errno(void)
{
	extern int errno;

	if (!__multithreaded)
		return &errno;
	return (*__lwp_priv_datap)->lwp_thread->thr_errp;
}

/*
 * __thr_multithreaded()
 *	Entry point for libthread startup to tell libc that we are
 *	multithreaded, and to slide new lwp descriptor under us.
 *	Caller must initialize lwpp.
 */
void
__thr_multithread(__lwp_desc_t *lwpp)
{
	__lwp_priv_datap = (__lwp_desc_t **)__lwp_private((void *)lwpp);
	__multithreaded = 1;
}

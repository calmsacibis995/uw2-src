/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:sys/lwpprivate.c	1.1"

/*
 * _lwp_setprivate and _lwp_getprivate.
 */

/*
 * void _lwp_setprivate(void *p)
 *	Set the private data pointer of the calling lwp.
 */
void
_lwp_setprivate(void *p)
{
	extern  void **__lwp_priv_datap;

	if (__lwp_priv_datap == 0)
		__thr_init();

	(void)__lwp_private(p);
}

/*
 * void *_lwp_getprivate(void)
 *	Return the private data pointer of the calling LWP.
 */
void *
_lwp_getprivate(void)
{
	extern  void **__lwp_priv_datap;

	if (__lwp_priv_datap == 0)
		__thr_init();

	return (*__lwp_priv_datap);
}

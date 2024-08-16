/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:i386/lib/libthread/sys/tinit.c	1.5.2.2"

#include <libthread.h>

/*
 * void _init(void)
 *
 *	Initialize the thread library.
 *
 * Calling/Exit state:
 *
 *	Called without holding any locks.
 *
 * Remarks:
 *	_init() is the very first function in libthread that is called
 *	by libc (e.g., from callInit()). Before reaching main() in the
 *	application code, many other libthread functions are called such
 *	as _thr_init, __thr_init, _lwp_getprivate, _thr_t0init, and
 *	_thr_sigt0init. (The function, __thr_init, is actually defined
 *	in libc.)
 */
void
_init(void)
{
	_thr_init();
}


/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libthread:i386/lib/libthread/sys/thr_t0init.c	1.5.5.9"

/*
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 */

#include <thread.h>
#include <libthread.h>
#include <trace.h>
#include <tls.h>
#include <sys/lwp.h>
#include <stdio.h>

/*
 * void _thr_init(void)
 *
 *	Initialize the thread library.
 *
 * Calling/Exit state:
 *
 *	Called without holding any locks.
 */
void
_thr_init(void)
{
	_thr_t0init();
	_thr_libcsync_init();

#ifdef TRACE
	{
		int	ctr;

		/*
		 * we'd like to call _thr_trace_init()  here, but we can't
		 * detect environment variables at this point; therefore, we
		 * initially set all trace categories to be traceable and the
		 * first trace event of the application will cause trace to be
		 * initialized.  This ensures that the initial thread will call
		 * _thr_trace_init()  while running on the initial LWP.
		 */

		for (ctr = 0; ctr < _thr_trace_category_count; ctr++) {
			_thr_trace_categories |= (1 << ctr);
		}
	}
#endif /* TRACE */

}

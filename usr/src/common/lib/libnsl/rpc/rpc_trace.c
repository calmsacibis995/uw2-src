/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpc_trace.c	1.2.2.3"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * rpc_trace.c
 */

#ifdef	TRACE
#include	<stdio.h>
#include	<sys/types.h>
#include	<sys/time.h>
#include	<sys/fcntl.h>
#include	<rpc/types.h>
#include	"trace.h"
#include	"rpc_mt.h"
#ifdef _REENTRANT
#include	<thread.h>
#endif /* _REENTRANT */

/*
 * last_time_written variable is not protected by locking because
 * the data corruption doesn't cause any serious problems.
 */

void
_rpc_trace(ev, d0, d1, d2, d3, d4, d5)
	u_long	ev, d0, d1, d2, d3, d4, d5;
{
	struct timeval	t;
	struct trace_record	tr;
	static pid_t	pid;
	static FILE	*fp = (FILE *) NULL;
	static time_t	last_time_written = (time_t) 0;
	static int count;

	if (fp == (FILE *) NULL) {
		char fname[100];

		MUTEX_LOCK(&__rpc_lock);
		if (fp == (FILE *) NULL) {
			pid	= getpid();
			sprintf(fname, "/tmp/rpc.trace.%d", pid);
			fp = _fopen(fname, "w");
			if (fp == (FILE *) NULL) {
				MUTEX_UNLOCK(&__rpc_lock);
				return;
			}
		}
		MUTEX_UNLOCK(&__rpc_lock);
	}

	tr.tr_time = 0;
	tr.tr_pid	= pid;
#ifdef _REENTRANT
	if (MULTI_THREADED)
	    tr.tr_tid	= thr_self();
	else
	    tr.tr_tid	= 0;
#endif /* _REENTRANT */
	tr.tr_tag	= ev;
	tr.tr_datum0	= d0;
	tr.tr_datum1	= d1;
	tr.tr_datum2	= d2;
	tr.tr_datum3	= d3;
	tr.tr_datum4	= d4;
	tr.tr_datum5	= d5;
	fwrite ((char *) &tr, sizeof (tr), 1, fp);
	fflush(fp);
}

#endif	/* TRACE */

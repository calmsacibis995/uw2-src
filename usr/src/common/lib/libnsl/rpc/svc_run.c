/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/svc_run.c	1.3.9.5"
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
 * svc_run.c
 *
 * This is the rpc server side idle loop
 * Wait for input, call server program.
 */
#include <rpc/rpc.h>
#include <errno.h>
#include <sys/poll.h>
#include <sys/types.h>
#include "trace.h"
#include <sys/syslog.h>
#include <unistd.h>
#include "rpc_mt.h"

static struct pollfd	*svc_pollset;

void
svc_run()
{
	int nfds;
	int dtbsize = _rpc_dtbsize();
	int i;

	trace1(TR_svc_run, 0);

	/* allocate svc_pollset array if not there already */
	if (!svc_pollset) {
		MUTEX_LOCK(&__svc_lock);
		if (!svc_pollset) {
			svc_pollset = (struct pollfd *)
			    calloc(FD_SETSIZE, sizeof(struct pollfd));
			if (!svc_pollset) {
				MUTEX_UNLOCK(&__svc_lock);
				(void) syslog(LOG_ERR,
				    gettxt("uxnsl:32", "%s: out of memory"),
				    "svc_run");
				trace1(TR_svc_run, 2);
				return;
			}
		}
		MUTEX_UNLOCK(&__svc_lock);
	}

	for (;;) {
		/*
		 * Check whether there is any server fd on which we may
		 * have to wait.
		 */
		MUTEX_LOCK(&__svc_lock);
		nfds = _rpc_select_to_poll(dtbsize, &svc_fdset, svc_pollset);
		MUTEX_UNLOCK(&__svc_lock);
		if (nfds == 0)
			break;	/* None waiting, hence quit */

		switch (i = poll(svc_pollset, nfds, -1)) {
		case -1:
			/*
			 * We ignore all errors except for EAGAIN, since
			 * we must sleep in case of low memory.  
			 * Otherwise, we continue with the assumption
			 * that errno was set by the signal handlers (or any
			 * other outside event) and not caused by poll().
			 */
			switch (errno) {
			case EAGAIN:
				sleep(5);
				continue;
			default:
				continue;
			}
		case 0:
			continue;
		default:
			svc_getreq_poll(svc_pollset, i);
		}
	}
	trace1(TR_svc_run, 1);
}

/*
 *	This function causes svc_run() to exit by telling it that it has no
 *	more work to do.
 */
void
svc_exit()
{
	trace1(TR_svc_exit, 0);
	/*
	 * we protect svc_fdset here to prevent someone from registering
	 * a new server handle.
	 */
	MUTEX_LOCK(&__svc_lock);
	FD_ZERO(&svc_fdset);
	MUTEX_UNLOCK(&__svc_lock);
	trace1(TR_svc_exit, 1);
}


#ifdef _REENTRANT

static struct pollfd	*svc_thr_pollset;

static	int		svc_num_serving;
static	int		svc_num_in_dispatch;
static	int		svc_iscreating;
static	int		svc_timeout;
static	int		svc_minthreads;
static	int		svc_maxthreads;
static	size_t		svc_stacksize;
static	thread_t	svc_thrid;
static	void		*svc_thr_start(void *);

int
svc_run_parallel(timeout, minthreads, maxthreads, stacksize)
int	timeout;
int	minthreads;
int	maxthreads;
size_t	stacksize;
{
	int		dtbsize = _rpc_dtbsize();
	int		rval;
	int 		nfds;
	int 		i;

	if ((minthreads <= 0) ||
	    (maxthreads <= 0) ||
	    (maxthreads <= minthreads) ||
	    /*
	     * stacksize must be either:
	     *  - zero, or
	     *  - value > thr_minstack()
	     * anything elase is no good.
	     *
	     * the test is for the negation of the following
	     * expression:
	     * (stacksize == 0 || stacksize > THR_MINSTACK())
	     */
	    (stacksize != 0 && stacksize <= THR_MINSTACK()))
		return(-1);

	MUTEX_LOCK(&__svc_lock);
	svc_minthreads = minthreads;
	svc_maxthreads = maxthreads;
	svc_timeout = timeout;
	svc_stacksize = stacksize;
	if (svc_num_serving == 0)
		svc_num_serving = 1;

	if (!svc_thr_pollset) {
		svc_thr_pollset = (struct pollfd *)
		    calloc(FD_SETSIZE, sizeof(struct pollfd));
		if (!svc_thr_pollset) {
			MUTEX_UNLOCK(&__svc_lock);
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:32", "%s: out of memory"),
			    "svc_run_parallel");
			return(-1);
		}
	}

	for (;;) {

		/*
		 * Check whether there is any server fd on which we may want
		 * to wait.
		 */
		nfds = _rpc_select_to_poll(dtbsize, &svc_fdset,
							svc_thr_pollset);
		MUTEX_UNLOCK(&__svc_lock);
		if (nfds == 0)
			break;	/* None waiting, hence quit */

		switch (i = poll(svc_thr_pollset, nfds, timeout)) {
		case -1:
			/*
			 * We ignore all errors except for EAGAIN, since
			 * we must sleep in case of low memory.  
			 * Otherwise, we continue with the assumption
			 * that it was set by the signal handlers (or any
			 * other outside event) and not caused by poll().
			 */
			switch (errno) {
			case EAGAIN:
				sleep(5);
				continue;
			default:
				continue;
			}
		case 0:
			/*
			 * if more than minimum threads are serving, then exit.
			 */
			MUTEX_LOCK(&__svc_lock);
			if (svc_num_serving > svc_minthreads) {
				svc_num_serving--;
				MUTEX_UNLOCK(&__svc_lock);
				THR_EXIT(NULL);
			}
			continue;
		default:
			MUTEX_LOCK(&__svc_lock);
			svc_num_in_dispatch++;

			if ((svc_iscreating == 0) &&
				(svc_num_in_dispatch == svc_num_serving) &&
					(svc_num_serving < svc_maxthreads)) {
				svc_iscreating = 1;
				MUTEX_UNLOCK(&__svc_lock);
				if ((rval = THR_CREATE(NULL, svc_stacksize,
				    svc_thr_start, NULL, NULL,
				    &svc_thrid)) != 0) {
					syslog(LOG_ERR,
					    "svc_run_parallel: thr_create: %s",
					    strerror(rval));
					MUTEX_LOCK(&__svc_lock);
				} else {
					MUTEX_LOCK(&__svc_lock);
					svc_num_serving++;
				}
				svc_iscreating = 0;
			}

			_svc_getreq_poll_parallel(svc_thr_pollset, i);

			svc_num_in_dispatch--;
		}
	}

	return(0);
}

void *
svc_thr_start(void *arg)
{
	(void)svc_run_parallel(	svc_timeout,
				svc_minthreads,
				svc_maxthreads,
				svc_stacksize);
}

#endif

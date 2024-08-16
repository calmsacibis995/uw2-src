/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpc_sel2poll.c	1.2.2.2"
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
 * rpc_sel2poll.c
 */

#include <sys/select.h>
#include <sys/types.h>
#include "trace.h"
#include <sys/time.h>
#include <sys/poll.h>

#define	MASKVAL	(POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND)

/*
 *	Given an fd_set pointer and the number of bits to check in it,
 *	initialize the supplied pollfd array for RPC's use (RPC only
 *	polls for input events).  We return the number of pollfd slots
 *	we initialized.
 */
int
_rpc_select_to_poll(fdmax, fdset, p0)
	int	fdmax;		/* number of bits we must test */
	fd_set	*fdset;		/* source fd_set array */
	struct pollfd	*p0;	/* target pollfd array */
{
	/* register declarations ordered by expected frequency of use */
	register long *in;
	register int j;		/* loop counter */
	register u_long b;	/* bits to test */
	register int n;
	register struct pollfd	*p = p0;

	/*
	 * For each fd, if the appropriate bit is set convert it into
	 * the appropriate pollfd struct.
	 */
	trace2(TR___rpc_select_to_poll, 0, fdmax);
	for (in = fdset->fds_bits, n = 0; n < fdmax; n += NFDBITS, in++)
		for (b = (u_long) *in, j = 0; b; j++, b >>= 1)
			if (b & 1) {
				p->fd = n + j;
				if (p->fd >= fdmax) {
					trace2(TR___rpc_select_to_poll,
							1, fdmax);
					return (p - p0);
				}
				p->events |= MASKVAL;
				p++;
			}

	trace2(TR___rpc_select_to_poll, 1, fdmax);
	return (p - p0);
}

/*
 *	Convert from timevals (used by select) to milliseconds (used by poll).
 */
int
_rpc_timeval_to_msec(t)
	register struct timeval	*t;
{
	int	t1, tmp;

	/*
	 *	We're really returning t->tv_sec * 1000 + (t->tv_usec / 1000)
	 *	but try to do so efficiently.  Note:  1000 = 1024 - 16 - 8.
	 */
	trace1(TR___rpc_timeval_to_msec, 0);
	tmp = t->tv_sec << 3;
	t1 = -tmp;
	t1 += t1 << 1;
	t1 += tmp << 7;
	if (t->tv_usec)
		t1 += t->tv_usec / 1000;

	trace1(TR___rpc_timeval_to_msec, 1);
	return (t1);
}

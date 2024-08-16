/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_look.c	1.2.5.6"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include <unistd.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

static int __t_look();

#ifdef t_look
#undef t_look
#endif

t_look(fd)
int fd;
{
	int retval;
	struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	/* Call the auxiliary function __t_look() whose code was removed
	 * from t_look() so that it could be called by other TLI functions
	 * that already held tiptr->lock.
	 */
	retval = __t_look(fd, tiptr);

	MUTEX_UNLOCK(&tiptr->lock);
	return(retval);
}

/*
 * This function implements TLI semantics.
 */

static int
__t_look(fd, tiptr)
int fd;
struct _ti_user *tiptr;
{
	struct strpeek strpeek;
	int retval;
	union T_primitives *pptr;
	long type;

	/*
	 * Assume that this function is called only from the TLI library
	 * and that we hold tiptr->lock for writing, so we can safely 
	 * update the _ti_user structure's control buffer and ti_lookflg.
	 */

	strpeek.ctlbuf.maxlen = sizeof(long);
	strpeek.ctlbuf.len = 0;
	strpeek.ctlbuf.buf = tiptr->ti_ctlbuf;
	strpeek.databuf.maxlen = 0;
	strpeek.databuf.len = 0;
	strpeek.databuf.buf = NULL;
	strpeek.flags = 0;

	if ((retval = ioctl(fd, I_PEEK, &strpeek)) < 0) {
		return(T_ERROR);
	}

	/*
	 * if something there and cnt part also there
	 */
	if (tiptr->ti_lookflg || (retval && (strpeek.ctlbuf.len
					     >= sizeof(long)))) {
		pptr = (union T_primitives *)strpeek.ctlbuf.buf;
		if (tiptr->ti_lookflg) {
			if (((type = *((long *)tiptr->ti_lookcbuf))
			     != T_DISCON_IND)
			 && (retval && (pptr->type == T_DISCON_IND))) {
				type = pptr->type;
				tiptr->ti_lookflg = 0;
			}
		} else
			type = pptr->type;

		switch(type) {

		case T_CONN_IND:
			return(T_LISTEN);

		case T_CONN_CON:
			return(T_CONNECT);

		case T_DISCON_IND:
			return(T_DISCONNECT);

		case T_DATA_IND:
		case T_UNITDATA_IND:
			return(T_DATA);

		case T_EXDATA_IND:
			return(T_EXDATA);

		case T_UDERROR_IND:
			return(T_UDERR);

		case T_ORDREL_IND:
			return(T_ORDREL);

		default:
			set_t_errno(TSYSERR);
			errno = EPROTO;
			return(-1);
		}
	}

	/*
	 * if something there put no control part
	 * it must be data
	 */
	if (retval && (strpeek.ctlbuf.len <= 0))
		return(T_DATA);

	/*
	 * if msg there and control
	 * part not large enough to determine type?
	 * it must be illegal TLI message
	 */
	if (retval && (strpeek.ctlbuf.len > 0)) {
		set_t_errno(TSYSERR);
		errno = EPROTO;
		return(-1);
	}

	return(0);
}

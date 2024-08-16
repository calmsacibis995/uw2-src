/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_sndrel.c	1.4.5.8"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stream.h>
#include <stropts.h>
#include <sys/tihdr.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

#ifdef t_sndrel
#undef t_sndrel
#endif

#pragma weak _xti_sndrel = t_sndrel

t_sndrel(fd)
int fd;
{
	int evt;
	struct T_ordrel_req orreq;
	struct strbuf ctlbuf;
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	if (tiptr->ti_servtype != T_COTS_ORD) {
		set_t_errno(TNOTSUPPORT);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	if ((tiptr->ti_state != T_INREL) && (tiptr->ti_state != T_DATAXFER)) {
                set_t_errno(TOUTSTATE);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }

	/*
	 *  __xti_look() must be called instead of _xti_look(),
	 *  since _xti_look()
	 *  calls _t_checkfd(), which acquires both _nsl_lock and tiptr->lock.
	 *  We have already called _t_checkfd(), so we need not do it again.  
	 *  By using this approach, we can hold our lock continuously.
	 */
	if ((evt = (int)__xti_look(fd, tiptr)) == -1) {
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	/*
	 *  If event, is T_DISCONNECT fail with TLOOK, else
	 *  continue with orderly release.
	 */
	if (evt == T_DISCONNECT) {
		set_t_errno(TLOOK);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	orreq.PRIM_type = T_ORDREL_REQ;
	ctlbuf.maxlen = sizeof(struct T_ordrel_req);
	ctlbuf.len = sizeof(struct T_ordrel_req);
	ctlbuf.buf = (caddr_t)&orreq;

	tiptr->ti_flags &= ~FLOWCNTL;

	if (putmsg(fd, &ctlbuf, NULL, 0) < 0) {
		if (errno == EAGAIN) {
                        set_t_errno(TFLOW);
                        tiptr->ti_flags |= FLOWCNTL;
                }
                else
                        set_t_errno(TSYSERR);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }
	tiptr->ti_state = TLI_NEXTSTATE(T_SNDREL, tiptr->ti_state);
	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}

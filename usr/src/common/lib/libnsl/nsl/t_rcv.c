/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_rcv.c	1.7.5.6"
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
#include <unistd.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

#ifdef t_rcv
#undef t_rcv
#endif

#pragma weak _xti_rcv = t_rcv

t_rcv(fd, buf, nbytes, flags)
int fd;
register char *buf;
unsigned nbytes;
int *flags;
{
	struct strbuf ctlbuf, rcvbuf;
	int retval, flg = 0;
	int msglen;
	register union T_primitives *pptr;
	register struct _ti_user *tiptr;
	int rcvbuflen;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	if (tiptr->ti_servtype == T_CLTS) {
		set_t_errno(TNOTSUPPORT);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (TLI_NEXTSTATE(T_RCV, tiptr->ti_state) == nvs) {
                set_t_errno(TOUTSTATE);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }
	
	/*
         * Check in lookbuf for stuff
	 */
	if (tiptr->ti_lookflg) {
		/*
		 * Beware - this is right!
		 *	If something in lookbuf then check
		 *	read queue to see if there is something there.
		 *	If there is something there and there is not a
		 * 	discon in lookbuf, then it must be a discon.
		 *      If so, fall through to get it off of queue.
		 *	I fall through to make sure it is a discon,
		 * 	instead of making check here.
		 *
		 *	If nothing in read queue then just return TLOOK.
		 */
		if ((retval = ioctl(fd, I_NREAD, &msglen)) < 0) {
			set_t_errno(TSYSERR);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
		if (retval) {
			if (*((long *)tiptr->ti_lookcbuf) == T_DISCON_IND) {
				set_t_errno(TLOOK);
				MUTEX_UNLOCK(&tiptr->lock);
				return(-1);
			}
		} else {
			set_t_errno(TLOOK);
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
	}

	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = tiptr->ti_ctlbuf;
	rcvbuf.maxlen = nbytes;
	rcvbuf.len = 0;
	rcvbuf.buf = buf;
	*flags = 0;

	/*
	 * data goes right in user buffer
	 */
	if ((retval = getmsg(fd, &ctlbuf, &rcvbuf, &flg)) < 0) {
		if (errno == EAGAIN)
			set_t_errno(TNODATA);
		else
			set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	if (rcvbuf.len == -1) rcvbuf.len = 0;


	if (ctlbuf.len > 0) {
		if (ctlbuf.len < sizeof(long)) {
			set_t_errno(TSYSERR);
			errno = EPROTO;
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}

		pptr = (union T_primitives *)ctlbuf.buf;

		switch(pptr->type) {

			case T_EXDATA_IND:
				*flags |= T_EXPEDITED;
				if (retval)
					tiptr->ti_flags |= EXPEDITED;

				/* flow thru */

			case T_DATA_IND:
				if ((ctlbuf.len < sizeof(struct T_data_ind)) ||
				    (tiptr->ti_lookflg)) {
					set_t_errno(TSYSERR);
					errno = EPROTO;
					MUTEX_UNLOCK(&tiptr->lock);
					return(-1);
				}
	
				if ((pptr->data_ind.MORE_flag) || retval)
					*flags |= T_MORE;
				if ((pptr->data_ind.MORE_flag) && retval)
					tiptr->ti_flags |= MORE;

				tiptr->ti_state
				   = TLI_NEXTSTATE(T_RCV, tiptr->ti_state);
				rcvbuflen = rcvbuf.len;
				MUTEX_UNLOCK(&tiptr->lock);
				return(rcvbuflen);
	
			case T_ORDREL_IND:
				if (tiptr->ti_lookflg) {
					set_t_errno(TSYSERR);
					errno = EPROTO;
					MUTEX_UNLOCK(&tiptr->lock);
					return(-1);
				}
				/* flow thru */

			case T_DISCON_IND:
				_t_putback(tiptr, rcvbuf.buf, rcvbuf.len,
					   ctlbuf.buf, ctlbuf.len);
				if (retval&MOREDATA) {
					ctlbuf.maxlen = 0;
					ctlbuf.len = 0;
					ctlbuf.buf = tiptr->ti_ctlbuf;
					rcvbuf.maxlen
					   = tiptr->ti_rcvsize - rcvbuf.len;
					rcvbuf.len = 0;
					rcvbuf.buf = tiptr->ti_lookdbuf
						    +tiptr->ti_lookdsize;
					*flags = 0;

					if ((retval
					     = getmsg(fd, &ctlbuf, &rcvbuf,
						      &flg)) < 0) {
						set_t_errno(TSYSERR);
						MUTEX_UNLOCK(&tiptr->lock);
						return(-1);
					}
					if (rcvbuf.len == -1) rcvbuf.len = 0;
					if (retval) {
						tiptr->ti_lookflg = 0;
						set_t_errno(TSYSERR);
						errno = EPROTO;
						MUTEX_UNLOCK(&tiptr->lock);
						return(-1);
					}
					tiptr->ti_lookdsize += rcvbuf.len;
				}
					
				set_t_errno(TLOOK);
				MUTEX_UNLOCK(&tiptr->lock);
				return(-1);
		
			default:
				break;
		}
	
		set_t_errno(TSYSERR);
		errno = EPROTO;
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	} else {
		if (!retval && (tiptr->ti_flags&MORE)) {
			*flags |= T_MORE;
			tiptr->ti_flags &= ~MORE;
		}
		if (retval)
			*flags |= T_MORE;

		/*
		 * If inside an ETSDU, set expedited flag and turn
		 * of internal version when reach end of "ETIDU".
		 */
		if (tiptr->ti_flags & EXPEDITED) {
			*flags |= T_EXPEDITED;
			if (!retval)
				tiptr->ti_flags &= ~EXPEDITED;
		}

		tiptr->ti_state = TLI_NEXTSTATE(T_RCV, tiptr->ti_state);
		rcvbuflen = rcvbuf.len;
		MUTEX_UNLOCK(&tiptr->lock);
		return(rcvbuflen);
	}
	/* NOT REACHED */
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_snd.c	1.3.9.4"
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

#ifdef t_snd
#undef t_snd
#endif

#pragma weak _xti_snd = t_snd

t_snd(fd, buf, nbytes, flags)
int fd;
register char *buf;
unsigned nbytes;
int flags;
{
	struct strbuf ctlbuf, databuf;
	struct T_data_req *datareq;
	unsigned tmpcnt, tmp;
	char *tmpbuf;
	register struct _ti_user *tiptr;
	int band;
	int ret;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	if (tiptr->ti_servtype == T_CLTS) {
		set_t_errno(TNOTSUPPORT);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	/*
	 *  Check the stream head for a pending T_DISCONNECT event.
	 *  If true, fail with TLOOK error.
	 */
	if (__xti_look(fd, tiptr) == T_DISCONNECT) {
		set_t_errno(TLOOK);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (flags & ~(T_MORE | T_EXPEDITED)) {
                set_t_errno(TBADFLAG);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }
	if (!(tiptr->ti_provider_flgs & T_SENDZERO) && nbytes == 0) {
		set_t_errno(TBADDATA);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	if ((tiptr->ti_tsdu > 0) && (nbytes > tiptr->ti_tsdu)) {
                set_t_errno(TBADDATA);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }
	if (TLI_NEXTSTATE(T_SND, tiptr->ti_state) == nvs) {
		set_t_errno(TOUTSTATE);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	datareq = (struct T_data_req *)tiptr->ti_ctlbuf;
	/*
         * Turn off the flow control flags since if they still apply they
         * will be reset below.
         */
        if (flags&T_EXPEDITED) {
                datareq->PRIM_type = T_EXDATA_REQ;
                tiptr->ti_flags &= ~FLOWCNTLEX;
                if (tiptr->ti_provider_flgs & T_EXPINLINE)
                        band = TI_NORMAL;
                else
                        band = TI_EXPEDITED;
        } else {
                datareq->PRIM_type = T_DATA_REQ;
                tiptr->ti_flags &= ~FLOWCNTL;
                band = TI_NORMAL;
        }

	ctlbuf.maxlen = sizeof(struct T_data_req);
	ctlbuf.len = sizeof(struct T_data_req);
	ctlbuf.buf = tiptr->ti_ctlbuf;
	tmp = nbytes;
	tmpbuf = buf;

	do {
		if ((tmpcnt = tmp) > tiptr->ti_maxpsz) {
			datareq->MORE_flag = 1;
			tmpcnt = tiptr->ti_maxpsz;
		} else {
			if (flags&T_MORE)
				datareq->MORE_flag = 1;
			else
				datareq->MORE_flag = 0;
		}

		databuf.maxlen = tmpcnt;
		databuf.len = tmpcnt;
		databuf.buf = tmpbuf;

#ifndef NO_IMPORT
		if (__putpmsg) {
			ret = putpmsg(fd, &ctlbuf, &databuf, band, MSG_BAND);
		} else {
			ret = putmsg(fd, &ctlbuf, &databuf, 0);
		}
#else
		ret = putpmsg(fd, &ctlbuf, &databuf, band, MSG_BAND);
#endif
		if (ret < 0) {
			if (nbytes == tmp) {
				if (errno == EAGAIN) {
                                        set_t_errno(TFLOW);
                                        if (flags&T_EXPEDITED)
                                                tiptr->ti_flags |= FLOWCNTLEX;
                                        else
                                                tiptr->ti_flags |= FLOWCNTL;
                                }
                                else
                                        set_t_errno(TSYSERR);
                                MUTEX_UNLOCK(&tiptr->lock);
                                return(-1);
			} else
				tiptr->ti_state
				   = TLI_NEXTSTATE(T_SND, tiptr->ti_state);
				MUTEX_UNLOCK(&tiptr->lock);
				return(nbytes - tmp);
		}
		tmp = tmp - tmpcnt;
		tmpbuf = tmpbuf + tmpcnt;
	} while (tmp);

	tiptr->ti_state = TLI_NEXTSTATE(T_SND, tiptr->ti_state);
	MUTEX_UNLOCK(&tiptr->lock);
	return(nbytes - tmp);
}

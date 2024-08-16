/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_sndudata.c	1.5.5.3"
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

#ifdef t_sndudata
#undef t_sndudata
#endif

#pragma weak _xti_sndudata = t_sndudata

t_sndudata(fd, unitdata)
int fd;
register struct t_unitdata *unitdata;
{
	register struct T_unitdata_req *udreq;
	char *buf;
	struct strbuf ctlbuf;
	int size;
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	if (tiptr->ti_servtype != T_CLTS) {
		set_t_errno(TNOTSUPPORT);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	if (tiptr->ti_state != T_IDLE) {
                set_t_errno(TOUTSTATE);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }
	if (!(tiptr->ti_provider_flgs & T_SENDZERO)
	 && (int)unitdata->udata.len == 0) {
		set_t_errno(TBADDATA);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	if ((tiptr->ti_tsdu > 0 )
	 && ((int)unitdata->udata.len > tiptr->ti_tsdu)) {
		set_t_errno(TBADDATA);
		errno = EPROTO;
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	buf = tiptr->ti_ctlbuf;
	udreq = (struct T_unitdata_req *)buf;
	udreq->PRIM_type = T_UNITDATA_REQ;
	udreq->DEST_length = unitdata->addr.len;
	udreq->DEST_offset = 0;
	udreq->OPT_length = unitdata->opt.len;
	udreq->OPT_offset = 0;
	size = sizeof(struct T_unitdata_req);

	if (unitdata->addr.len) {
		_t_aligned_copy(buf, unitdata->addr.len, size,
			     unitdata->addr.buf, &udreq->DEST_offset);
		size = udreq->DEST_offset + udreq->DEST_length;
	}
	if (unitdata->opt.len) {
		_t_aligned_copy(buf, unitdata->opt.len, size,
			     unitdata->opt.buf, &udreq->OPT_offset);
		size = udreq->OPT_offset + udreq->OPT_length;
	}

	if (size > tiptr->ti_ctlsize) {
		set_t_errno(TSYSERR);
		errno = EIO;
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = size;
	ctlbuf.buf = buf;

	tiptr->ti_flags &= ~FLOWCNTL;
	if (putmsg(fd, &ctlbuf, (unitdata->udata.len?
				 (struct strbuf *)&unitdata->udata: NULL), 0)
	    < 0) {
		if (errno == EAGAIN) {
                        set_t_errno(TFLOW);
                        tiptr->ti_flags |= FLOWCNTL;
                }

                else
                        set_t_errno(TSYSERR);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
	}

	tiptr->ti_state = TLI_NEXTSTATE(T_SNDUDATA, tiptr->ti_state);
	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}

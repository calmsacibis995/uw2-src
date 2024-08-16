/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_rcvudata.c	1.5.5.7"
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
#include <string.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

#ifdef t_rcvudata
#undef t_rcvudata
#endif

#pragma weak _xti_rcvudata = t_rcvudata

t_rcvudata(fd, unitdata, flags)
int fd;
register struct t_unitdata *unitdata;
int *flags;
{
	struct strbuf ctlbuf;
	int retval, flg = 0;
	register union T_primitives *pptr;
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

	/*
         * check if there is something in look buffer
	 */
	if (tiptr->ti_lookflg) {
		set_t_errno(TLOOK);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	ctlbuf.maxlen = tiptr->ti_ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = tiptr->ti_ctlbuf;
	*flags = 0;

	/*
	 * data goes right in user buffer
	 */
	if ((retval = getmsg(fd, &ctlbuf,
			     (struct strbuf *)&unitdata->udata, &flg)) < 0) {
		if (errno == EAGAIN)
			set_t_errno(TNODATA);
		else
			set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	if (unitdata->udata.len == -1) unitdata->udata.len = 0;

	/*
	 * is there control piece with data?
	 */
	if (ctlbuf.len > 0) {
		if (ctlbuf.len < sizeof(long)) {
			set_t_errno(TSYSERR);
			errno = EPROTO;
			unitdata->udata.len = 0;
			MUTEX_UNLOCK(&tiptr->lock);
			return(-1);
		}
	
		pptr = (union T_primitives *)ctlbuf.buf;
	
		switch(pptr->type) {
	
			case T_UNITDATA_IND:
				if ((ctlbuf.len < sizeof(struct T_unitdata_ind))
				 || (pptr->unitdata_ind.OPT_length
				     && (ctlbuf.len
					 < (pptr->unitdata_ind.OPT_length
				           + pptr->unitdata_ind.OPT_offset)))) {
					set_t_errno(TSYSERR);
					errno = EPROTO;
					unitdata->udata.len = 0;
					MUTEX_UNLOCK(&tiptr->lock);
					return(-1);
				}
				if (((unitdata->addr.maxlen != 0)
				     && (pptr->unitdata_ind.SRC_length
				         > unitdata->addr.maxlen))
				 || ((unitdata->opt.maxlen != 0)
				     && (pptr->unitdata_ind.OPT_length
				         > unitdata->opt.maxlen))) {
					set_t_errno(TBUFOVFLW);
					unitdata->udata.len = 0;
					MUTEX_UNLOCK(&tiptr->lock);
					return(-1);
				}
	
				if (retval)
					*flags |= T_MORE;
	
				unitdata->addr.len = 0;
				unitdata->opt.len = 0;
				if (unitdata->addr.maxlen != 0) {
					memcpy(unitdata->addr.buf, ctlbuf.buf +
					 pptr->unitdata_ind.SRC_offset,
					 (int)pptr->unitdata_ind.SRC_length);
					unitdata->addr.len =
					 pptr->unitdata_ind.SRC_length;
				}
				if (unitdata->opt.maxlen != 0) {
					memcpy(unitdata->opt.buf,
					 ctlbuf.buf +
					 pptr->unitdata_ind.OPT_offset,
					 (int)pptr->unitdata_ind.OPT_length);
					unitdata->opt.len =
					 pptr->unitdata_ind.OPT_length;
				}
	
				tiptr->ti_state
				   = TLI_NEXTSTATE(T_RCVUDATA, tiptr->ti_state);
				MUTEX_UNLOCK(&tiptr->lock);
				return(0);
	
			case T_UDERROR_IND:
				_t_putback(tiptr, unitdata->udata.buf, 0,
					   ctlbuf.buf, ctlbuf.len);
				unitdata->udata.len = 0;
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
		unitdata->addr.len = 0;
		unitdata->opt.len = 0;
		/*
		 * only data in message no control piece
		 */
		if (retval)
			*flags = T_MORE;

		tiptr->ti_state = TLI_NEXTSTATE(T_RCVUDATA, tiptr->ti_state);
		MUTEX_UNLOCK(&tiptr->lock);
		return(0);
	}
}

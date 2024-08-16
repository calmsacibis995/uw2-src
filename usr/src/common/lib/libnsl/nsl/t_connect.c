/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_connect.c	1.7.4.9"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include <fcntl.h>
#include <signal.h>
#include "_import.h"

extern _snd_conn_req(), _rcv_conn_con();
extern struct _ti_user *_t_checkfd();

#ifdef t_connect
#undef t_connect
#endif

#pragma weak _xti_connect = t_connect

t_connect(fd, sndcall, rcvcall)
int fd;
struct t_call *sndcall;
struct t_call *rcvcall;
{
	int fctlflg;
	register struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

        if (tiptr->ti_servtype == T_CLTS) {
                set_t_errno(TNOTSUPPORT);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }

	/*
	 *  Must be in T_IDLE state, else fail with TOUTSTATE error.
	 */

	if (tiptr->ti_state != T_IDLE) {
		set_t_errno(TOUTSTATE);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	/*
	 * Acquire tiptr->lock for writing, so that we may safely
	 * update the _ti_user structure, including flags and buffers.
	 */

	if (_snd_conn_req(fd, tiptr, sndcall) < 0) {
		switch (get_t_errno()) {
                case TLOOK:
                        tiptr->ti_state
                           = TLI_NEXTSTATE(T_CONNECT2, tiptr->ti_state);
                        break;
		default:
			break;
		}
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if ((fctlflg = fcntl(fd, F_GETFL, 0)) < 0) {
		set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (fctlflg & (O_NDELAY | O_NONBLOCK)) {
		tiptr->ti_state = TLI_NEXTSTATE(T_CONNECT2, tiptr->ti_state);
		set_t_errno(TNODATA);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	if (_rcv_conn_con(fd, tiptr, rcvcall) < 0) {
		switch (get_t_errno()) {
                case TLOOK:
                        tiptr->ti_state
                           = TLI_NEXTSTATE(T_CONNECT2, tiptr->ti_state);
                        break;
                case TBUFOVFLW:
                        tiptr->ti_state
                           = TLI_NEXTSTATE(T_CONNECT1, tiptr->ti_state);
                        break;
                case TNODATA:  /* this can happen if signal interrupt */
                        tiptr->ti_state
                           = TLI_NEXTSTATE(T_CONNECT2, tiptr->ti_state);
                }

		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	tiptr->ti_state = TLI_NEXTSTATE(T_CONNECT1, tiptr->ti_state);
	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}

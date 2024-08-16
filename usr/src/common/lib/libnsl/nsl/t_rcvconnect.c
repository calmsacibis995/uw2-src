/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_rcvconnect.c	1.3.5.6"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include <sys/param.h>
#include "_import.h"

extern int get_t_errno();
extern int _rcv_conn_con();
extern struct _ti_user *_t_checkfd();

#ifdef t_rcvconnect
#undef t_rcvconnect
#endif

#pragma weak _xti_rcvconnect = t_rcvconnect

t_rcvconnect(fd, call)
int fd;
struct t_call *call;
{
	register struct _ti_user *tiptr;
	int retval;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

        if (tiptr->ti_servtype == T_CLTS) {
                set_t_errno(TNOTSUPPORT);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }

	if (tiptr->ti_state != T_OUTCON) {
                set_t_errno(TOUTSTATE);
                MUTEX_UNLOCK(&tiptr->lock);
                return(-1);
        }

	if (((retval = _rcv_conn_con(fd, tiptr, call)) == 0)
	 || (get_t_errno() == TBUFOVFLW)) {
		tiptr->ti_state = TLI_NEXTSTATE(T_RCVCONNECT, tiptr->ti_state);
	}
	MUTEX_UNLOCK(&tiptr->lock);
	return(retval);
}

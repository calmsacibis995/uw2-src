/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_getstate.c	1.4.7.4"
#ident	"$Header: $"

#include <errno.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include <stdio.h>
#include <sys/signal.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

#ifdef t_getstate
#undef t_getstate
#endif

#pragma weak _xti_getstate = t_getstate

t_getstate(fd)
int fd;
{
	register struct _ti_user *tiptr;
	int state;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	switch (tiptr->ti_state) {

	case T_UNBND:
	case T_IDLE:
	case T_INCON:
	case T_OUTCON:
	case T_DATAXFER:
	case T_INREL:
	case T_OUTREL:
		state = tiptr->ti_state;
		MUTEX_UNLOCK(&tiptr->lock);
		return(state);
	case T_FAKE:
	default:
		set_t_errno(TSTATECHNG);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
}

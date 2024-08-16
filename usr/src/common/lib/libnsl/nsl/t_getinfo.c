/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_getinfo.c	1.5.5.8"
#ident	"$Header: $"

#include <errno.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <sys/xti.h>
#include <signal.h>
#include "_import.h"

extern struct _ti_user *_t_checkfd();

#ifdef t_getinfo
#undef t_getinfo
#endif

t_getinfo(fd, info)
int fd;
register struct t_info *info;
{
	struct T_info_ack inforeq;
	int retlen;
	sigset_t		oldmask;
	sigset_t		newmask;
	struct _ti_user *tiptr;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	inforeq.PRIM_type = T_INFO_REQ;

	if (!_t_do_ioctl(fd, (caddr_t)&inforeq, sizeof(struct T_info_req), TI_GETINFO, &retlen)) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
		
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	if (retlen != sizeof(struct T_info_ack)) {
		errno = EIO;
		set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	info->addr = inforeq.ADDR_size;
	info->options = inforeq.OPT_size;
	info->tsdu = inforeq.TSDU_size;
	info->etsdu = inforeq.ETSDU_size;
	info->connect = inforeq.CDATA_size;
	info->discon = inforeq.DDATA_size;
	info->servtype = inforeq.SERV_type;
        /* ***************************************************
         *
         * Do NOT assign anything to info->flags in this function,
         * or the calling program will dump core!
         *
         * ***************************************************/

	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_close.c	1.5.5.7"
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
#include <stdlib.h>
#include <signal.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();
extern _null_tiptr();

#ifdef t_close
#undef t_close
#endif

#pragma weak _xti_close = t_close

t_close(fd)
int fd;
{
	register struct _ti_user *tiptr;
	sigset_t		oldmask;
	sigset_t		newmask;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		close(fd);
		return(-1);
	}

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if (tiptr->ti_rcvbuf != NULL)
		(void)free(tiptr->ti_rcvbuf);
	if (tiptr->ti_lookdbuf != NULL)
		(void)free(tiptr->ti_lookdbuf);
	(void)free(tiptr->ti_ctlbuf);
	(void)free(tiptr->ti_lookcbuf);

	_null_tiptr(tiptr);

	close(fd);
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}

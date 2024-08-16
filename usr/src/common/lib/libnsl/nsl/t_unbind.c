/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_unbind.c	1.3.9.8"
#ident	"$Header: $"

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include "nsl_mt.h"
#include <sys/stream.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/timod.h>
#include <sys/xti.h>
#include <signal.h>
#include <unistd.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

#ifdef t_unbind
#undef t_unbind
#endif

#pragma weak _xti_unbind = t_unbind

t_unbind(fd)
int fd;
{
	struct _ti_user 	*tiptr;
	struct T_unbind_req 	*unbind_req;
	sigset_t		oset;
	int			ret_t_do_ioctl;
	sigset_t		oldmask;
	sigset_t		newmask;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	/* Must be in T_IDLE state. */
	if (tiptr->ti_state != T_IDLE) {
		set_t_errno(TOUTSTATE);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if (_t_is_event(fd, tiptr)) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	unbind_req = (struct T_unbind_req *)tiptr->ti_ctlbuf;
	unbind_req->PRIM_type = T_UNBIND_REQ;

	/*
	 * Warning!!  This is a patch to make sure that the ioctl() TI_UNBIND
	 * request is not interrupted, since it cannot be restarted
	 * reliably.
	 * The macros MT_MASKSIGS and MT_UNMASKSIGS are defined in <mt.h>.
	 * They allow this library to work both with and without libthread
	 * and require function calls only when t_unbind.c is compiled with
	 * _REENTRANT defined.
	 */

	/* Mask SIGWAITING and SIGLWP before making unbind request.  */
	MT_MASKSIGS(&oset);

	ret_t_do_ioctl = _t_do_ioctl(fd, (caddr_t)unbind_req,
				     sizeof(struct T_unbind_req),
			 	     TI_UNBIND, NULL);

	/* Unmask SIGWAITING and SIGLWP after making unbind request. */
	MT_UNMASKSIGS(&oset);

	/*
	 * End of patch
	 */

	if (!ret_t_do_ioctl) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	if (ioctl(fd, I_FLUSH, FLUSHRW) < 0) {
		set_t_errno(TSYSERR);
		MUTEX_UNLOCK(&tiptr->lock);
		return(-1);
	}

	/*
	 * clear more data in TSDU bit
	 */
	tiptr->ti_flags &= ~MORE;

	tiptr->ti_state = TLI_NEXTSTATE(T_UNBIND, tiptr->ti_state);
	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}

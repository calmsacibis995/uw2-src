/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/t_bind.c	1.3.9.9"
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
#include <signal.h>
#include <string.h>
#include "_import.h"


extern struct _ti_user *_t_checkfd();

#ifdef t_bind
#undef t_bind
#endif

t_bind(fd, req, ret)
int fd;
struct _xti_bind *req;
struct _xti_bind *ret;
{
	char			*buf;
	struct T_bind_req	*ti_bind;
	int			size,
				ret_t_do_ioctl;
	struct _ti_user		*tiptr;
	sigset_t		oset;
	sigset_t		oldmask;
	sigset_t		newmask;

	if ((tiptr = _t_checkfd(fd)) == NULL) {
		return(-1);
	}

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	/*
	 * Protect our _ti_user structure and its buffers while we
	 * update and use it.
	 */
	buf = tiptr->ti_ctlbuf;
	ti_bind = (struct T_bind_req *)buf;
	size = sizeof(struct T_bind_req);

	ti_bind->PRIM_type = O_T_BIND_REQ;
	ti_bind->ADDR_length = (req == NULL? 0: req->addr.len);
	ti_bind->ADDR_offset = 0;
	ti_bind->CONIND_number = (req == NULL? 0: req->qlen);


	if (ti_bind->ADDR_length) {
		_t_aligned_copy(buf, (int)ti_bind->ADDR_length, size,
			     req->addr.buf, &ti_bind->ADDR_offset);
		size = ti_bind->ADDR_offset + ti_bind->ADDR_length;
	}
			       
	/*
	 * Warning!!  This is a patch to make sure that the ioctl() TI_BIND
	 * request is not interrupted, since it cannot be restarted
	 * reliably.
	 * The macros MT_MASKSIGS and MT_UNMASKSIGS are defined in <mt.h>.
	 * They allow this library to work both with and without libthread
	 * and require function calls only when t_bind.c is compiled with
	 * _REENTRANT defined.
	 */

	/* Mask SIGWAITING and SIGLWP before making bind request.  */
	MT_MASKSIGS(&oset);

	ret_t_do_ioctl = _t_do_ioctl(fd, buf, size, TI_BIND, NULL);

	/* Unmask SIGWAITING and SIGLWP after making bind request. */
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

	tiptr->ti_ocnt = 0;
	tiptr->ti_qlen = ti_bind->CONIND_number;

	tiptr->ti_state = TLI_NEXTSTATE(T_BIND, tiptr->ti_state);

	if (ret != NULL) {
		ret->addr.len = 0;
		if (ret->addr.maxlen != 0) {
			if (ti_bind->ADDR_length > ret->addr.maxlen) {
				set_t_errno(TBUFOVFLW);
				MUTEX_UNLOCK(&tiptr->lock);
				return(-1);
			}
			/*
			 * buf is in our _ti_user, so we must hold lock to
			 * read it.
			 */
			memcpy(ret->addr.buf,
			 (char *)(buf + ti_bind->ADDR_offset),
			 (int)ti_bind->ADDR_length);
			ret->addr.len = ti_bind->ADDR_length;
			ret->qlen = ti_bind->CONIND_number;
		}
	}

	MUTEX_UNLOCK(&tiptr->lock);
	return(0);
}

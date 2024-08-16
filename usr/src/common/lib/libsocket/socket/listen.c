/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)libsocket:common/lib/libsocket/socket/listen.c	1.8.11.5"
#ident	"$Header: $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stream.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/socketvar.h>
#include <sys/socket.h>
#include <sys/xti.h>		/* REQUIRED - XPG4 */
#include <sys/sockmod.h>
#include <sys/signal.h>

/* We make the socket module do the unbind,
 * if necessary, to make the timing window
 * of error as small as possible.
 */
int
listen(s, qlen)
	int			s;
	int			qlen;
{
	char			*buf;
	struct T_bind_req	*bind_req;
	struct _si_user		*siptr;
	sigset_t		oldmask;
	sigset_t		newmask;

	if ((siptr = _s_checkfd(s)) == NULL)
		return (-1);

	if (siptr->udata.servtype == T_CLTS) {
		errno = EOPNOTSUPP;
		return (-1);
	}

	buf = siptr->ctlbuf;
	bind_req = (struct T_bind_req *)buf;

	bind_req->PRIM_type = T_BIND_REQ;
	bind_req->ADDR_offset = sizeof (*bind_req);
	bind_req->CONIND_number = qlen;

	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		int	family;

		/*
		 * Must have been explicitly bound in the UNIX domain.
		 */
		if ((family = _s_getfamily(siptr)) == AF_UNIX) {
			errno = EINVAL;
			return (-1);
		}

		(void)memcpy(buf + bind_req->ADDR_offset, (caddr_t)&family,
				sizeof (short));
		bind_req->ADDR_length = 0;
	} else	bind_req->ADDR_length = siptr->udata.addrsize;

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if (!_s_do_ioctl(s, siptr->ctlbuf, sizeof (*bind_req) +
				bind_req->ADDR_length, SI_LISTEN, NULL)) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return (-1);
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	siptr->udata.so_options |= SO_ACCEPTCONN;

	return (0);
}

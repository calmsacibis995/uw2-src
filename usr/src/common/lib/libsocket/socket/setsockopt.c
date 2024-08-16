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

#ident	"@(#)libsocket:common/lib/libsocket/socket/setsockopt.c	1.6.11.6"
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
#include "../libsock_mt.h"
#include <sys/timod.h>
#include <sys/socket.h>
#include <sys/xti.h>		/* REQUIRED - XPG4 */
#include <sys/sockmod.h>
#include <sys/signal.h>

int
setsockopt(s, level, optname, optval, optlen)
	int			s;
	int			level;
	int			optname;
	const void		*optval;
	size_t			optlen;
{
	register struct _si_user	*siptr;

	if ((siptr = _s_checkfd(s)) == NULL)
		return (-1);

	if (level == SOL_SOCKET && optname == SO_TYPE) {
		errno = ENOPROTOOPT;
		return (-1);
	}
	return (_setsockopt(siptr, level, optname, optval, optlen));
}

int
_setsockopt(siptr, level, optname, optval, optlen)
	struct _si_user		*siptr;
	int			level;
	int			optname;
	char			*optval;
	int			optlen;
{
	char			*buf;
	struct T_optmgmt_req	*opt_req;
	int			s;
	int			size;
	struct opthdr		*opt;
	sigset_t		oldmask;
	sigset_t		newmask;

	s = siptr->fd;

	buf = siptr->ctlbuf;
	opt_req = (struct T_optmgmt_req *)buf;
	opt_req->PRIM_type = T_OPTMGMT_REQ;
	opt_req->OPT_length = sizeof (*opt) + optlen;
	opt_req->OPT_offset = sizeof (*opt_req);
	opt_req->MGMT_flags = T_NEGOTIATE;

	opt = (struct opthdr *)(buf + sizeof (*opt_req));
	opt->level = level;
	opt->name = optname;
	opt->len = optlen;
	(void)memcpy((caddr_t)opt + sizeof (*opt), optval, optlen);

	size = opt_req->OPT_offset + opt_req->OPT_length;

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if (!_s_do_ioctl(s, buf, size, TI_OPTMGMT, 0)) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return (-1);
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	return (0);
}

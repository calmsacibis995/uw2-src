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

#ident	"@(#)libsocket:common/lib/libsocket/socket/shutdown.c	1.7.10.5"
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

int
shutdown(s, how)
	int			s;
	int			how;
{
	struct   _si_user	*siptr;
	sigset_t		oldmask;
	sigset_t		newmask;

	if ((siptr = _s_checkfd(s)) == (struct _si_user *)NULL)
		return (-1);

	if (how < 0 || how > 2) {
		errno = EINVAL;
		return (-1);
	}

	if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
		if (_s_getudata(s, &siptr) < 0)
			return (-1);
		if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
			errno = ENOTCONN;
			return (-1);
		}
	}

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if (!_s_do_ioctl(s, &how, sizeof (how), SI_SHUTDOWN, 0)) {
		if (errno != EPIPE) {
			(void) sigprocmask(SIG_SETMASK, &oldmask, 
					   (sigset_t *)NULL);
			return (-1);
		} else	errno = 0;
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	/*
	 * If we got EPIPE back from the ioctl, then we can
	 * no longer talk to sockmod. The best we can do now
	 * is set our local state and hope the user doesn't
	 * use read/write.
	 */
	if (how == 0 || how == 2)
		siptr->udata.so_state |= SS_CANTRCVMORE;
	if (how == 1 || how == 2)
		siptr->udata.so_state |= SS_CANTSENDMORE;

	return (0);
}

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

#ident	"@(#)libsocket:common/lib/libsocket/socket/accept.c	1.12.9.11"
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

#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mkdev.h>
#include <errno.h>
#include <sys/stream.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/tihdr.h>
#include <sys/socketvar.h>
#include <sys/socket.h>
#include <sys/xti.h>		/* REQUIRED - XPG4 */
#include <sys/signal.h>
#include <sys/sockmod.h>
#include <sys/stat.h>
#include <sys/sockio.h>
#include <fcntl.h>
#include <syslog.h>
#include "../socketabi.h"

int
accept(s, addr, addrlen)
	int		s;
	struct sockaddr *addr;
	size_t		*addrlen;

{
	register struct _si_user	*siptr;

	if ((siptr = _s_checkfd(s)) == NULL)
		return (-1);

	if (siptr->udata.servtype == T_CLTS) {
		errno = EOPNOTSUPP;
		return (-1);
	}

	/*
	 * Make sure a listen() has been done
	 * actually if the accept() has not been done, then the
	 * effect will be that the user blocks forever.
	 */
	if ((siptr->udata.so_options & SO_ACCEPTCONN) == 0) {
		errno = EINVAL;
		return (-1);
	}

	return (_accept(siptr, addr, addrlen));
}

int
_accept(siptr, addr, addrlen)
	struct _si_user		*siptr;
	struct sockaddr		*addr;
	int			*addrlen;
{

	struct T_conn_res	*cres;
	struct _si_user		*nsiptr;
	int			s;
	int			s2;
	int			retval;
	struct netconfig	*net;
	dev_t			needev;
	union T_primitives	*pptr;
	struct strfdinsert	strfdinsert;
	int			flg;
	struct stat		statd;
	struct strbuf		ctlbuf;
	void			*nethandle;
	sigset_t		oldmask;
	sigset_t		newmask;

	flg = 0;
	s = siptr->fd;

	/*
	 * Get/wait for the T_CONN_IND.
	 */
	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	/*
	 * We don't expect any data, so no data
	 * buffer is needed.
	 */
	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if ((retval = getmsg(s, &ctlbuf, NULL, &flg)) < 0) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		if (errno == EAGAIN)
			errno = EWOULDBLOCK;
		return (-1);
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	/*
	 * did I get entire message?
	 */
	if (retval) {
		errno = EIO;
		return (-1);
	}

	/*
	 * is ctl part large enough to determine type
	 */
	if (ctlbuf.len < sizeof (long)) {
		errno = EPROTO;
		return (-1);
	}

	pptr = (union T_primitives *)ctlbuf.buf;
	switch (pptr->type) {
		case T_CONN_IND:
			if (ctlbuf.len < (sizeof (struct T_conn_ind)+
				pptr->conn_ind.SRC_length)) {
				errno = EPROTO;
				return (-1);
			}
			if (addr && addrlen) {
				*addrlen = _s_cpaddr(siptr, addr, *addrlen,
					ctlbuf.buf + pptr->conn_ind.SRC_offset,
					pptr->conn_ind.SRC_length);
			}
			break;

		default:
			errno = EPROTO;
			return (-1);
	}

	/*
	 * get the device file to open.
	 */
	if (fstat(s, &statd) < 0)
		return (-1);
	needev = major(statd.st_rdev);

	/*
	 * loop through each entry in netconfig
	 * until one matches.
	 */
	if ((nethandle = setnetconfig()) == NULL) {
		int save_errno;

		save_errno = errno;
		(void)syslog(LOG_ERR,
		    gettxt("uxnsl:174", "_accept: setnetconfig failed"));
		errno = save_errno;
		return (-1);
	}
	while ((net = getnetconfig(nethandle)) != NULL) {
		if (net->nc_semantics != NC_TPI_COTS &&
			net->nc_semantics != NC_TPI_COTS_ORD)
			continue;
		if (stat(net->nc_device, &statd) < 0)
			continue;
		if (minor(statd.st_rdev) == needev)
			break;
	}
	if (net == NULL) {
		endnetconfig(nethandle);
		errno = ENODEV;
		return (-1);
	}

	/*
	 * Open a new instance to do the accept on
	 *
	 * Note that we have lost the protocol number(if one was
	 * specified) with the listening endpoint, so we assume the
	 * transport provider makes a good enough copy of all the
	 * endpoints details, when the new one is created.
	 */
	if ((nsiptr = _s_open(net->nc_device, 0)) == NULL) {
		endnetconfig(nethandle);
		return (-1);
	}
	endnetconfig(nethandle);

	s2 = nsiptr->fd;

	/*
	 * must be bound for TLI.
	 */
	if (_bind(nsiptr, NULL, 0, NULL, NULL) < 0) {
		_s_close(nsiptr);
		return (-1);
	}

	cres = (struct T_conn_res *)siptr->ctlbuf;
	cres->PRIM_type = T_CONN_RES;
	cres->OPT_length = 0;
	cres->OPT_offset = 0;
	cres->SEQ_number = pptr->conn_ind.SEQ_number;

	strfdinsert.ctlbuf.maxlen = siptr->ctlsize;
	strfdinsert.ctlbuf.len = sizeof (*cres);
	strfdinsert.ctlbuf.buf = (caddr_t)cres;

	strfdinsert.databuf.maxlen = 0;
	strfdinsert.databuf.len = -1;
	strfdinsert.databuf.buf = NULL;

	strfdinsert.fildes = s2;
	strfdinsert.offset = sizeof (long);
	strfdinsert.flags = 0;

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if (ioctl(s, I_FDINSERT, &strfdinsert) < 0) {
		_s_close(nsiptr);
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return (-1);
	}

	if (!_s_is_ok(siptr, T_CONN_RES)) {
		_s_close(nsiptr);
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return (-1);
	}

	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	/*
	 * New socket must have attributes of the
	 * accepting socket.
	 */
	nsiptr->udata.so_state |= SS_ISCONNECTED;
	nsiptr->udata.so_options = siptr->udata.so_options & ~SO_ACCEPTCONN;

	/*
	 * Make the ownership of the new socket the
	 * same as the original.
	 */
	retval = 0;
	if (ioctl(s, SIOCGPGRP, &retval) == 0) {
		if (retval != 0) {
			(void)ioctl(nsiptr->fd, SIOCSPGRP, &retval);
		}
	} else	{
		(void)syslog(LOG_ERR,
		    gettxt("uxnsl:175", "accept: SIOCGPGRP failed errno %d"),
		    errno);
		errno = 0;
	}

	/*
	 * The accepted socket inherits the non-blocking and SIGIO
	 * attributes of the accepting socket.
	 */
	if ((flg = fcntl(s, F_GETFL, 0)) < 0) {
		(void)syslog(LOG_ERR,
		    gettxt("uxnsl:176", "accept: fcntl: F_GETFL failed %d"),
		    errno);
		errno = 0;
	} else	{
		flg &= (FREAD|FWRITE|FASYNC|FNDELAY);
		(void)fcntl(nsiptr->fd, F_SETFL, flg);
	}

	return (s2);
}

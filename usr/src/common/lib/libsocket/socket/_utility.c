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

#ident	"@(#)libsocket:common/lib/libsocket/socket/_utility.c	1.20.16.13"
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

#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/mkdev.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <sys/socket.h>
#include <sys/xti.h>
#include <sys/tihdr.h>
#include <sys/sockmod.h>
#include <sys/uio.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <sys/un.h>
#include <sys/resource.h>
#include <poll.h>
#include <fcntl.h>
#include <syslog.h>
#include "../socketabi.h"
#include "../libsock_mt.h"

#define DEFAULT_MIN_SDS	64

static struct _si_user	*find_silink();
static struct _si_user	*add_silink();
static int		 delete_silink();
static int		tlitosyserr();
static int		get_msg_slice();
static int		 recvaccrights();
static int		 send_clts_rights();
static int		 send_cots_rights();
static int		 _recvmsg();
static int		 msgpeek();
static int		 _s_alloc_bufs();

/*
 * Global, used to enable debugging.
 * May be read and written atomically.
 * Not supported per-thread.
 */
int			_s_sockdebug;

/*
 * Static, used to manage per-socket _si_user structures.
 * Protected by _s_si_user_lock.
 */
static struct _si_user		**sdp;
static int			nsd;

/*
 * The following two string arrays map a number as specified
 * by a user of sockets, to the string as would be returned
 * by a call to getnetconfig().
 *
 * They are used by _s_match();
 *
 * proto_sw contains protocol entries for which there is a corresponding
 * /dev device. All others would presumably use raw IP and download the
 * desired protocol.
 */
static const char *proto_sw[] = {
	"",
	"icmp",		/* 1 = ICMP */
	"",
	"",
	"",
	"",
	"tcp",		/* 6 = TCP */
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"udp",		/* 17 = UDP */
};

static const char *family_sw[] = {
	"-",		/* 0 = AF_UNSPEC */
	"loopback",	/* 1 = AF_UNIX */
	"inet",		/* 2 = AF_INET */
	"implink",	/* 3 = AF_IMPLINK */
	"pup",		/* 4 = AF_PUP */
	"chaos",	/* 5 = AF_CHAOS */
	"ns",		/* 6 = AF_NS */
	"nbs",		/* 7 = AF_NBS */
	"ecma",		/* 8 = AF_ECMA */
	"datakit",	/* 9 = AF_DATAKIT */
	"ccitt",	/* 10 = AF_CCITT */
	"sna",		/* 11 = AF_SNA */
	"decnet",	/* 12 = AF_DECnet */
	"dli",		/* 13 = AF_DLI */
	"lat",		/* 14 = AF_LAT */
	"hylink",	/* 15 = AF_HYLINK */
	"appletalk",	/* 16 = AF_APPLETALK */
	"nit",		/* 17 = AF_NIT */
	"ieee802",	/* 18 = AF_802 */
	"osi",		/* 19 = AF_OSI */
	"x25",		/* 20 = AF_X25 */
	"osinet",	/* 21 = AF_OSINET */
	"gosip",	/* 22 = AF_GOSIP */
};

/*
 * Checkfd - checks validity of file descriptor
 */
struct _si_user *
_s_checkfd(s)
	register int		s;
{
	struct _si_user		*siptr;

	if (s < 0 ) {
		errno = EBADF;	/* Return now and not try to re-constitute */
		return(NULL);	/* from an obviously invalid pointer. */
	}			
	if ((siptr = find_silink(s)) != NULL)
		return (siptr);

	errno = 0;

	/*
	 * Maybe the descripter is valid, but we did an exec()
	 * and lost the information. Try to re-constitute the info.
	 */
	SOCKDEBUG((struct _si_user *)NULL,
		"_s_checkfd: s %d: Not found, trying to reconstitute\n", s);
	if (_s_getudata(s, &siptr) < 0)
		return (NULL);

	return	(siptr);
}

/*
 * Do common open stuff.
 */
struct _si_user *
_s_open(device, protocol)
	register char		*device;
	int			protocol;
{
	register int 		s;
	int			retval;
	int			push_count;
	struct   _si_user	*siptr;

	SOCKDEBUG((struct _si_user *)NULL, "_s_open: %s\n", device);
	if ((s = open(device, O_RDWR)) < 0)
		return (NULL);

	/*
	 * Is module already pushed
	 */
	if ((retval = _ioctl(s, I_FIND, "sockmod")) < 0) {
		(void)close(s);
		return (NULL);
	}

	/*
	 * do PUSH at most 100 times...it should only take few...
	 */
	push_count=100; 
	if (!retval)
		while ((_ioctl(s, I_PUSH, "sockmod") < 0) && push_count--) {
			if (errno != EINTR)
			{
				(void)close(s);
				return (NULL);
			}
		}

	/*
	 * Set stream head close time to 0.
	 */
	retval = 0;
	(void)_ioctl(s, I_SETCLTIME, &retval);

	/*
	 * Turn on SIGPIPE stream head write
	 * option.
	 */
	if (_ioctl(s, I_SWROPT, SNDPIPE) < 0) {
		(void)close(s);
		return (NULL);
	}

	/*
	 * Get a new library entry and sync it
	 * with sockmod.
	 */
	siptr = NULL;
	if (_s_getudata(s, &siptr) < 0) {
		(void)close(s);
		return (NULL);
	}

	if (protocol) {
		/*
		 * Need to send down the protocol number.
		 */

		if (_setsockopt(siptr, SOL_SOCKET, SO_PROTOTYPE,
				(caddr_t)&protocol, sizeof (protocol)) < 0) {
			int save_errno;

			save_errno = errno;
			(void)syslog(LOG_ERR,
			    gettxt("uxnsl:171",
				"_s_open: setsockopt: SO_PROTOTYPE failed"));
			errno = save_errno;
		}
	}
	siptr->family = -1;	/* No family (yet) */
	return (siptr);
}

/*
 * Match config entry for protocol
 * requested.
 */
struct netconfig *
_s_match(family, type, proto, nethandle)
	register int			family;
	register int			type;
	register int			proto;
	void				**nethandle;
{
	register struct netconfig	*net;
	register struct netconfig	*maybe;
	register char			*oproto;

	if (family < 0 || family >= sizeof (family_sw)/sizeof (char *) ||
				proto < 0 || proto >= IPPROTO_MAX)  {
		errno = EPROTONOSUPPORT;
		return (NULL);
	}
	if (proto) {
		if (proto >= sizeof (proto_sw)/sizeof (char *))
			oproto = "";
		else	oproto = (char *)proto_sw[proto];
	}

	/*
	 * Loop through each entry in netconfig
	 * until one matches or we reach the end.
	 */
	if ((*nethandle = setnetconfig()) == NULL) {
	
		int save_errno;

		save_errno = errno;
		(void)syslog(LOG_ERR,
		    gettxt("uxnsl:172",
			"_s_match: setnetconfig failed"));
		errno = save_errno;
		return (NULL);
	}

	maybe = NULL;
	while ((net = getnetconfig(*nethandle)) != NULL) {
		if (net->nc_semantics == NC_TPI_COTS_ORD)
			net->nc_semantics = NC_TPI_COTS;
		if (proto) {
			if (strcmp(net->nc_protofmly, family_sw[family]) == 0 &&
				    net->nc_semantics == type &&
				    strcmp(net->nc_proto, oproto) == 0)
				break;

			if (strcmp(net->nc_protofmly, family_sw[family]) == 0 &&
				    type == SOCK_RAW &&
				    net->nc_semantics == SOCK_RAW &&
				    strcmp(net->nc_proto, NC_NOPROTO) == 0 &&
				    maybe == NULL)
				maybe = net;	/* in case no exact match */

			continue;
		} else	{
			if (strcmp(net->nc_protofmly, family_sw[family]) == 0 &&
					net->nc_semantics == type) {
				break;
			}
		}
	}
	if (net == NULL && maybe)
		net = maybe;

	if (net == NULL) {
		endnetconfig(*nethandle);
		errno = EPROTONOSUPPORT;
		return (NULL);
	}

	return (net);
}

int
_s_getudata(s, siptr)
register int				s;
register struct _si_user		**siptr;
{
	struct si_udata			udata;
	register struct _si_user	*nsiptr;
	int				retlen;
	int				pid;
	int				retval;
	sigset_t			oldmask;
	sigset_t			newmask;

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if (!_s_do_ioctl(s, (caddr_t)&udata, sizeof (struct si_udata),
			SI_GETUDATA, &retlen)) {
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		if (*siptr)
			_s_close(*siptr);
		return (-1);
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);

	if (retlen != sizeof (struct si_udata)) {
		errno = EPROTO;
		if (*siptr)
			_s_close(*siptr);
		return (-1);
	}

	if (*siptr == NULL) {
		/*
		 * Allocate a link and initialize it.
		 */
		nsiptr = add_silink(s);
		if (_s_alloc_bufs(nsiptr, &udata) < 0)
			return (-1);
		nsiptr->udata = udata;		/* structure copy */
		nsiptr->family = -1;
		nsiptr->flags = 0;

		*siptr = nsiptr;

		/*
		 * Get SIGIO and SIGURG disposition
		 * and cache them.
		 */
		retval = 0;
		if (_ioctl(s, I_GETSIG, &retval) < 0 &&
				errno != EINVAL) {

			int save_errno;
			
			save_errno = errno;
			(void)syslog(LOG_ERR,
			    gettxt("uxnsl:173",
				"ioctl: I_GETSIG failed %d"),
			    errno);
			errno = save_errno;
			return (-1);
		}

		errno = 0;
		if (retval & (S_RDNORM|S_WRNORM))
			nsiptr->flags |= S_SIGIO;

		if (retval & (S_RDBAND|S_BANDURG))
			nsiptr->flags |= S_SIGURG;

		return (0);
	} else	(*siptr)->udata = udata;

	SOCKDEBUG(*siptr, "_s_getudata: siptr: %x\n", *siptr);

	return (0);
}

/*
 * Get access rights and associated data.
 *
 * Only UNIX domain supported.
 *
 * Returns:
 *	>0	Number of bytes read on success
 *	-1	If an error occurred.
 */
static int
recvaccrights(siptr, msg, fmode)
	register struct _si_user	*siptr;
	register struct msghdr		*msg;
	register int			fmode;

{
	register int			i;
	register int			nfd;
	int				*fdarray;
	struct strrecvfd		pipe;
	int				retval;
	int				count;
	struct sockaddr_un		addr;

	if (_s_getfamily(siptr) != AF_UNIX) {
		errno = EOPNOTSUPP;
		return (-1);
	}

	/*
	 * First get the pipe channel.
	 */
	if (_ioctl(siptr->fd, I_RECVFD, &pipe) < 0)
		return (-1);

	/*
	 * To ensure the following operations are atomic
	 * as far as the user is concerned, we reset
	 * O_NDELAY if it is on.
	 */
	if (fmode & (O_NDELAY | O_NONBLOCK)) {
		if (_fcntl(siptr->fd, F_SETFL, fmode & ~O_NDELAY) < 0) {
			retval = -1;
			goto recv_rights_done;
		}
	}

	/*
	 * We do the same whether the flags say MSG_PEEK or
	 * not.
	 */
	SOCKDEBUG(siptr, "recvaccrights: getting access rights\n", 0);

	/*
	 * Dispose of rights, copying them into the users
	 * buffer if possible.
	 */
	fdarray = (int *)msg->msg_accrights;
	nfd = msg->msg_accrightslen/sizeof (int);
	msg->msg_accrightslen = 0;
	i = 0;
	for (;;) {
		struct strrecvfd	stfd;

		retval = 0;
		if (_ioctl(pipe.fd, I_RECVFD, &stfd) < 0)
			break;
		else	{
			SOCKDEBUG(siptr, "recvaccrights: got fd %d\n", stfd.fd);
			if (i != nfd) {
				fdarray[i] = stfd.fd;
				msg->msg_accrightslen += sizeof (int);
				i++;
			} else	{
				(void)close(stfd.fd);
			}
		}
	}

	if (errno == EBADMSG) {
		/*
		 * We have read all the access rights, get any data.
		 */
		errno = 0;
		if (siptr->udata.servtype == T_CLTS) {
			/*
			 * First get the source address.
			 */
			(void)memset((caddr_t)&addr, 0, sizeof (addr));
			if (read(pipe.fd, (caddr_t)&addr, sizeof (addr))
						!= sizeof (addr)) {
				errno = EPROTO;
				retval = -1;
				goto recv_rights_done;
			}
			if (msg->msg_name && msg->msg_namelen) {
				if (msg->msg_namelen > sizeof (addr))
					msg->msg_namelen = sizeof (addr);
				(void)memcpy(msg->msg_name, (caddr_t)&addr,
						msg->msg_namelen);
			}
		}

		for (i = 0; i < msg->msg_iovlen; i++) {
			count = read(pipe.fd, msg->msg_iov[i].iov_base,
				msg->msg_iov[i].iov_len);
			SOCKDEBUG(siptr, "recvaccrights: got %d bytes\n",
							count);
			if (count > 0)
				retval += count;
			if (count == 0) {
				/* EOF */
				errno = 0;
				break;
			}
		}
	} else	{
		/*
		 * No data.
		 */
		if (errno == ENXIO) {
			errno = 0;
			retval = 0;
		}
	}

recv_rights_done:
	(void)close(pipe.fd);
	if (fmode & (O_NDELAY | O_NONBLOCK))
		(void)_fcntl(siptr->fd, F_SETFL, fmode);
	return (retval);
}

/*
 * Peeks at a message. If no messages are
 * present it will block in a poll().
 * Note ioctl(I_PEEK) does not block.
 *
 * Returns:
 *	0	On success
 *	-1	On error. In particular, EBADMSG is returned if access
 *		are present.
 */
static int
msgpeek(s, ctlbuf, rcvbuf, fmode)
	register int		s;
	register struct strbuf	*ctlbuf;
	register struct strbuf	*rcvbuf;
	register int		fmode;
{
	register int		retval;
	struct strpeek		strpeek;

	strpeek.ctlbuf.buf = ctlbuf->buf;
	strpeek.ctlbuf.maxlen = ctlbuf->maxlen;
	strpeek.ctlbuf.len = 0;
	strpeek.databuf.buf = rcvbuf->buf;
	strpeek.databuf.maxlen = rcvbuf->maxlen;
	strpeek.databuf.len = 0;
	strpeek.flags = 0;

	for (;;) {
		if ((retval = _ioctl(s, I_PEEK, &strpeek)) < 0)
			return (-1);

		if (retval == 1) {
			ctlbuf->len = strpeek.ctlbuf.len;
			rcvbuf->len = strpeek.databuf.len;
			return (0);
		} else	if ((fmode & (O_NDELAY | O_NONBLOCK)) == 0) {
			/*
			 * Sit in a poll()
			 */
			struct pollfd	fds[1];

			fds[0].fd = s;
			fds[0].events = POLLIN;
			fds[0].revents = 0;
			for (;;) {
				if (poll(fds, 1L, -1) < 0)
					return (-1);
				if (fds[0].revents != 0)
					break;
			}
		} else	{
			errno = EAGAIN;
			return (-1);
		}
	}
}

/*
 * Receive a message according to flags.
 *
 * Returns:
 *	count 	on success
 *	-1 	on error.
 */
static int
_recvmsg(siptr, msg, flags)
	struct _si_user	*siptr;
	struct msghdr	*msg;
	int		flags;
{
	int		fmode;
	int		s;
	int		len;
	int		pos;
	int		i;
	char		*addr;
	struct strbuf	ctlbuf;
	struct strbuf	rcvbuf;
	int 		addrlen;
	int		flg;
	int		errflg = 0;
	sigset_t	oldmask;
	sigset_t	newmask;

	s = siptr->fd;
	for (i = 0, len = 0; i < msg->msg_iovlen; i++)
		len += msg->msg_iov[i].iov_len;

	if (len == 0 && msg->msg_accrightslen == 0)
		return (0);

	if (msg->msg_iovlen > 1) {
		if ((rcvbuf.buf = calloc(1, len)) == NULL) {
			errno = ENOMEM;
			return (-1);
		}
	} else	rcvbuf.buf = msg->msg_iov[0].iov_base;

	fmode = _fcntl(siptr->fd, F_GETFL, 0);

tryagain:
	rcvbuf.maxlen = len;
	rcvbuf.len = 0;

	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	if (flags & MSG_OOB) {
		/*
		 * Handles the case when MSG_PEEK is set
		 * or not.
		 */
		if (! _s_do_ioctl(s, rcvbuf.buf, rcvbuf.maxlen, flags,
								&rcvbuf.len)) {
			rcvbuf.len = -1;
			errflg = 1;
			goto rcvout;
		}
		SOCKDEBUG(siptr, "_recvmsg: Got %d bytes OOB data\n",
						rcvbuf.len);
	} else if (flags & MSG_PEEK) {
		if (msgpeek(s, &ctlbuf, &rcvbuf, fmode) < 0) {
			errflg = 1;
			if (errno == EBADMSG) {
				errflg = 0;
				rcvbuf.len = recvaccrights(siptr, msg, fmode);
			}
			goto rcvout;
		}
	} else	{
		flg = 0;

		/*
		 * Have to prevent spurious SIGPOLL signals
		 * which can be caused by the mechanism used
		 * to cause a SIGURG.
		 */
		(void) sigemptyset(&newmask);
		(void) sigaddset(&newmask, SIGPOLL);
		(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
		if (getmsg(s, &ctlbuf, &rcvbuf, &flg) < 0) {
			(void) sigprocmask(SIG_SETMASK, &oldmask, 
					   (sigset_t *)NULL);
			errflg=1;
			if (errno == EBADMSG) {
				errflg = 0;
				rcvbuf.len = recvaccrights(siptr, msg, fmode);
			}
			goto rcvout;
		}
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	}

	if (rcvbuf.len == -1)
		rcvbuf.len = 0;

	if (ctlbuf.len == sizeof (struct T_exdata_ind) &&
				*(long *)ctlbuf.buf == T_EXDATA_IND &&
				rcvbuf.len == 0) {
		/*
		 * Must be the message indicating the position
		 * of urgent data in the data stream - the user
		 * should not see this.
		 */
		if (flags & MSG_PEEK) {
			/*
			 * Better make sure it goes.
			 */
			flg = 0;
			(void) sigemptyset(&newmask);
			(void) sigaddset(&newmask, SIGPOLL);
			(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);
			(void)getmsg(s, &ctlbuf, &rcvbuf, &flg);
			(void) sigprocmask(SIG_SETMASK, &oldmask, 
					   (sigset_t *)NULL);
		}
		goto tryagain;
	}

	/*
	 * Copy it all back as per the users
	 * request.
	 */
	if (msg->msg_iovlen > 1) {
		register int count;

		for (i=pos=0, len=rcvbuf.len; i < msg->msg_iovlen; i++) {
			count = _s_min(msg->msg_iov[i].iov_len, len);
			(void)memcpy(msg->msg_iov[i].iov_base, &rcvbuf.buf[pos],
						count);
			pos += count;
			len -= count;
			if (len == 0)
				break;
		}
	}

	/*
	 * Copy in source address if requested.
	 */
rcvout:
	if (errflg == 0 && msg->msg_name && msg->msg_namelen) {
		if (siptr->udata.servtype == T_CLTS) {
			if (ctlbuf.len != 0) {
				register struct T_unitdata_ind *udata_ind;

				udata_ind = (struct T_unitdata_ind *)ctlbuf.buf;
				msg->msg_namelen = _s_cpaddr(siptr,
					msg->msg_name,
					msg->msg_namelen,
					udata_ind->SRC_offset + ctlbuf.buf,
					udata_ind->SRC_length);

			}
		} else	{
			addrlen = msg->msg_namelen;
			if (_s_getname(siptr, msg->msg_name, &addrlen, REMOTENAME) < 0) {
				msg->msg_namelen = 0;
			}
			msg->msg_namelen = addrlen;
		}
	}

	if (msg->msg_iovlen > 1)
		(void)free(rcvbuf.buf);

	if (errflg)
		return (-1);

	return (rcvbuf.len);
}

/*
 * Common receive code.
 */
int
_s_soreceive(siptr, msg, flags)
	struct _si_user		*siptr;
	register struct msghdr	*msg;
	register int		flags;
{
	if (siptr->udata.so_state & SS_CANTRCVMORE)
		return (0);

	if (siptr->udata.servtype == T_COTS ||
			siptr->udata.servtype == T_COTS_ORD) {
		if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
			if (_s_getudata(siptr->fd, &siptr) < 0)
				return (-1);
			if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
				errno = ENOTCONN;
				return (-1);
			}
		}
	}

	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		/*
		 * Need to bind it for TLI.
		 */
		if (_bind(siptr, NULL, 0, NULL, NULL) < 0)
			return (-1);
	}

	return(_recvmsg(siptr, msg, flags));

}

/*
 * Common send code.
 */
int
_s_sosend(siptr, msg, flags)
	struct _si_user		*siptr;
	register struct msghdr	*msg;
	register int		flags;
{
	register int		s;
	register int		i;
	register int		len;
	register int		retval = 0;
	struct strbuf		ctlbuf;
	struct strbuf		databuf;

	if (siptr->udata.so_state & SS_CANTSENDMORE) {
		/* Send SIGPIPE to the calling thread or process */
		(void) MT_KILL_SELF(SIGPIPE);
		errno = EPIPE;
		return (-1);
	}

	s = siptr->fd;
	if ((siptr->udata.servtype == T_CLTS && msg->msg_namelen <= 0) ||
					siptr->udata.servtype != T_CLTS) {
		if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
			if (_s_getudata(s, &siptr) < 0)
				return (-1);
			if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
				if (siptr->udata.servtype == T_CLTS)
					errno = EDESTADDRREQ;
				else	errno = ENOTCONN;
				return (-1);
			}
		}
	}

	if ((siptr->udata.so_state & SS_ISBOUND) == 0) {
		/*
		 * Need to bind it for TLI.
		 */
		if (_bind(siptr, NULL, 0, NULL, NULL) < 0)
			return (-1);
	}

	for (i= 0, len = 0; i < msg->msg_iovlen; i++)
		len += msg->msg_iov[i].iov_len;

	if (flags & MSG_DONTROUTE) {
		int	val;

		val = 1;
		if (_setsockopt(siptr, SOL_SOCKET, SO_DONTROUTE, &val,
				sizeof (val)) < 0)
			return (-1);
	}

	/*
	 * Access rights only in UNIX domain.
	 */
	if (msg->msg_accrightslen) {
		if (_s_getfamily(siptr) != AF_UNIX) {
			errno = EOPNOTSUPP;
			retval = -1;
			goto sndout;
		}
	}

	if (flags & MSG_OOB) {
		/*
		 * If the socket is SOCK_DGRAM or
		 * AF_UNIX which we know is not to support
		 * MSG_OOB or the TP does not support the
		 * notion of expedited data then we fail.
		 *
		 * Otherwise we hope that the TP knows
		 * what to do.
		 */
		if (_s_getfamily(siptr) == AF_UNIX ||
				siptr->udata.servtype == T_CLTS ||
				siptr->udata.etsdusize == 0) {
			errno = EOPNOTSUPP;
			retval = -1;
			goto sndout;
		}
	}

	if (siptr->udata.servtype == T_CLTS) {
		register struct T_unitdata_req	*udata_req;
		register char			*dbuf;
		register char			*tmpbuf;
		register int			pos;
		register int			tmpcnt;
		struct ux_dev			ux_dev;

		if (len < 0 || len > siptr->udata.tidusize) {
			errno = EMSGSIZE;
			retval = -1;
			goto sndout;
		}

		if ((siptr->udata.so_state & SS_ISCONNECTED) == 0) {
			switch (_s_getfamily(siptr)) {
			case AF_INET:
				if (msg->msg_namelen !=
						sizeof (struct sockaddr_in)){
					errno = EINVAL;
					retval = -1;
				}
				break;

			case AF_UNIX:
				/*
				 * Checked below.
				 */
				break;

			default:
				if (msg->msg_namelen > siptr->udata.addrsize){
					errno = EINVAL;
					retval = -1;
				}
				break;
			}
			if (retval < 0)
				goto sndout;
		}

		if (msg->msg_namelen > 0 && _s_getfamily(siptr) == AF_UNIX) {
			struct stat		rstat;
			struct sockaddr_un 	*un;

			un = (struct sockaddr_un *)msg->msg_name;

			if (msg->msg_namelen > sizeof (*un) ||
					(i = _s_uxpathlen(un)) ==
						sizeof (un->sun_path)) {
				errno = EINVAL;
				retval = -1;
				goto sndout;
			}
			un->sun_path[i] = 0;

			/*
			 * Substitute the user supplied address with the
			 * one that will have actually got bound to.
			 */
			if (un->sun_family != AF_UNIX) {
				errno = EINVAL;
				retval = -1;
				goto sndout;
			}

			/*
			 * stat the file.
			 */
			if (stat((caddr_t)un->sun_path, &rstat) < 0) {
				retval = -1;
				goto sndout;
			}

			if ((rstat.st_mode & S_IFMT) != S_IFIFO) {
				errno = ENOTSOCK;
				retval = -1;
				goto sndout;
			}

			(void)memset((caddr_t)&ux_dev, 0, sizeof (ux_dev));
			ux_dev.dev = rstat.st_dev;
			ux_dev.ino = rstat.st_ino;

			msg->msg_name = (char *)&ux_dev;
			msg->msg_namelen = sizeof (ux_dev);
		}

		if (msg->msg_iovlen > 1) {
			/*
			 * Have to make one buffer
			 */
			if ((dbuf = calloc(1, len)) == NULL) {
				errno = ENOMEM;
				retval = -1;
				goto sndout;
			}

			for (i= 0, pos = 0; i < msg->msg_iovlen; i++) {
				(void)memcpy(&dbuf[pos],
						msg->msg_iov[i].iov_base,
						msg->msg_iov[i].iov_len);
				pos += msg->msg_iov[i].iov_len;
			}
		} else	dbuf = msg->msg_iov[0].iov_base;

		if (msg->msg_accrightslen) {
			retval = send_clts_rights(siptr, msg, dbuf, len);
			goto sndout;
		}

		tmpbuf = siptr->ctlbuf;
		udata_req = (struct T_unitdata_req *)tmpbuf;
		udata_req->PRIM_type = T_UNITDATA_REQ;
		udata_req->DEST_length = _s_min(msg->msg_namelen,
				siptr->udata.addrsize);
		udata_req->DEST_offset = 0;
		udata_req->OPT_length = 0;
		tmpcnt = sizeof (*udata_req);

		if ((int)udata_req->DEST_length > 0 && tmpbuf != (char *)NULL) {
			_s_aligned_copy(tmpbuf, udata_req->DEST_length, tmpcnt,
				msg->msg_name, &udata_req->DEST_offset);
			tmpcnt += udata_req->DEST_length;
		}

		ctlbuf.len = tmpcnt;
		ctlbuf.buf = tmpbuf;

		databuf.len = len == 0 ? -1 : len;
		databuf.buf = dbuf;

		SOCKDEBUG(siptr, "_s_sosend: sending %d bytes\n", databuf.len);
		if ((retval = putmsg(s, &ctlbuf, &databuf, 0)) < 0) {
			if (errno == EAGAIN)
				errno = ENOMEM;
		}
		if (msg->msg_iovlen > 1)
			(void)free(dbuf);
		if (retval >= 0) {
			retval = databuf.len == -1 ? 0 : databuf.len;
		}
		goto sndout;
	} else	{
		register struct T_data_req	*data_req;
		register int			tmp;
		register int			tmpcnt;
		register int			firsttime;
		register int			error;
		char				*tmpbuf;
		char		*pos = NULL; /* These variables are used */
		int		left = 0;    /* to avoid statics in      */
		int		ii = 0;      /* get_msg_slice().         */

		if (len == 0) {
			retval = 0;
			goto sndout;
		}

		if (msg->msg_accrightslen) {
			retval = send_cots_rights(siptr, msg);
			goto sndout;
		}

		data_req = (struct T_data_req *)siptr->ctlbuf;

		ctlbuf.len = sizeof (*data_req);
		ctlbuf.buf = siptr->ctlbuf;

		tmp = len;
		firsttime = 0;
		while (tmpcnt = get_msg_slice(msg, &tmpbuf,
				siptr->udata.tidusize, firsttime,
				pos, &left, &ii)) {
			if (flags & MSG_OOB) {
				data_req->PRIM_type = T_EXDATA_REQ;
				if ((tmp - tmpcnt) != 0)
					data_req->MORE_flag = 1;
				else	data_req->MORE_flag = 0;
			} else	{
				data_req->PRIM_type = T_DATA_REQ;
			}

			if (data_req->PRIM_type == T_DATA_REQ) {
				/*
				 * Performance Optimization, don't bother
				 * with header.
				 */
				SOCKDEBUG(siptr,
				"_s_sosend: writing %d bytes\n", tmpcnt);
				error = write(s, tmpbuf, tmpcnt);
			} else	{
				/*
				 * Urgent data.
				 */
				databuf.len = tmpcnt;
				databuf.buf = tmpbuf;
				SOCKDEBUG(siptr,
				"_s_sosend: sending %d bytes\n", tmpcnt);
				error = putmsg(s, &ctlbuf, &databuf, 0);
			}
			if (error < 0) {
				if (len == tmp) {
					if (errno == EAGAIN)
						errno = ENOMEM;
					retval = -1;
					goto sndout;
				} else	{
					errno = 0;
					retval = len - tmp;
					goto sndout;
				}
			}
			firsttime = 1;
			tmp -= tmpcnt;
		}
		retval = len - tmp;
	}
sndout:
	if (flags & MSG_DONTROUTE) {
		int	val;

		val = 0;
		_setsockopt(siptr, SOL_SOCKET, SO_DONTROUTE, &val,
			sizeof (val));
	}
	if (retval < 0) {
		if (errno == ENXIO || errno == EIO)
			errno = EPIPE;
		return (-1);
	}
	return	(retval);
}

static int
send_clts_rights(siptr, msg, data, datalen)
	register struct _si_user	*siptr;
	register struct msghdr		*msg;
	register char			*data;
	register int			datalen;
{
	int				pipefd[2];
	int				*rights;
	int				retval;
	int				i;
	struct sockaddr_un		un_addr;
	int				addrlen;

	/*
	 * Get a pipe.
	 */
	if (pipe(pipefd) < 0)
		return (-1);

	/*
	 * Link the transport.
	 */
	if (_s_do_ioctl(siptr->fd, msg->msg_name, msg->msg_namelen,
				SI_TCL_LINK, NULL) == 0)  {
		retval = -1;
		goto send_clts_done;
	}

	/*
	 * Send one end of the pipe.
	 */
	if (_ioctl(siptr->fd, I_SENDFD, pipefd[1]) < 0) {
		retval = -1;
		goto send_clts_done;
	}

	/*
	 * Send the fd's.
	 */
	SOCKDEBUG(siptr, "send_clts_rights: nmbr of fd's %d\n",
				msg->msg_accrightslen/sizeof (int));
	rights = (int *)msg->msg_accrights;
	for (i = 0; i < msg->msg_accrightslen/sizeof (int); i++) {
		if (_ioctl(pipefd[0], I_SENDFD, rights[i]) < 0) {
			retval = -1;
			goto send_clts_done;
		}
	}

	/*
	 * Send our address.
	 */
	(void)memset((caddr_t)&un_addr, 0, sizeof (un_addr));
	addrlen = sizeof (un_addr);
	if (_s_getname(siptr, (struct sockaddr *)&un_addr, &addrlen, LOCALNAME) < 0)
		goto send_clts_done;

	addrlen = sizeof (un_addr);
	if (write(pipefd[0], (caddr_t)&un_addr, addrlen) != addrlen) {
		errno = EPROTO;
		goto send_clts_done;
	}

	/*
	 * Send the data.
	 */
	if (datalen) {
		if (write(pipefd[0], data, datalen) != datalen) {
			retval = -1;
			goto send_clts_done;
		} else	retval = datalen;
	}

	/*
	 * Unlink the transport.
	*/
send_clts_done:
	(void)_s_do_ioctl(siptr->fd, NULL, 0, SI_TCL_UNLINK, NULL);
	(void)close(pipefd[0]);
	(void)close(pipefd[1]);

	return (retval);
}

static
send_cots_rights(siptr, msg)
	register struct _si_user	*siptr;
	register struct msghdr		*msg;
{
	int				pipefd[2];
	ulong				intransit;
	int				*fds;
	int				retval;
	int				i;

	/*
	 * Get a pipe.
	 */
	if (pipe(pipefd) < 0)
		return (-1);

	/*
	 * Ensure nothing in progress.
	 */
	intransit = 0;
	for (;;) {
		if (_s_do_ioctl(siptr->fd, (caddr_t)&intransit,
			sizeof (intransit), SI_GETINTRANSIT, &i) == 0) {
			retval = -1;
			goto send_cots_done;
		}
		if (i != sizeof (intransit)) {
			errno = EPROTO;
			retval = -1;
			goto send_cots_done;
		}
		if (intransit != 0)
			(void)sleep(1);
		else	break;
	}

	/*
	 * Send pipe fd.
	 */
	if (_ioctl(siptr->fd, I_SENDFD, pipefd[1]) < 0) {
		retval = -1;
		goto send_cots_done;
	}

	/*
	 * Send the fd's.
	 */
	fds = (int *)msg->msg_accrights;
	for (i = 0; i < msg->msg_accrightslen/sizeof (int); i++) {
		if (_ioctl(pipefd[0], I_SENDFD, fds[i]) < 0) {
			retval = -1;
			goto send_cots_done;
		}
	}
	/*
	 * Send the data.
	 */
	if (msg->msg_iovlen) {
		retval = 0;
		for (i = 0; i < msg->msg_iovlen; i++) {
			if (write(pipefd[0], msg->msg_iov[i].iov_base,
					msg->msg_iov[i].iov_len) !=
					msg->msg_iov[i].iov_len) {
				errno = EPROTO;
				retval = -1;
				goto send_cots_done;
			}
			retval += msg->msg_iov[i].iov_len;
		}
	}

send_cots_done:
	(void)close(pipefd[0]);
	(void)close(pipefd[1]);

	if (retval >= 0)
		errno = 0;

	return (retval);
}

/*
 * On return, ptr points at the next slice of
 * data of askedfor size. Returns the actual
 * amount.
 */
static int
get_msg_slice(msg, ptr, askedfor, firsttime, pos, leftp, ip)
	register struct msghdr	*msg;
	register char		**ptr;
	register int		askedfor;
	register int		firsttime;
	char			*pos;   /* These arguments replace   */
	int			*leftp; /* the static variables pos, */
	int			*ip;    /* left, and i.              */

{
	register int		count;

	if (!firsttime) {
		if (msg->msg_iovlen <= 0) {
			*ptr = NULL;
			return (0);
		}
		*ip = 0;
		*leftp = msg->msg_iov[*ip].iov_len;
		pos = msg->msg_iov[*ip].iov_base;
	}
again:
	if (*leftp) {
		if (*leftp > askedfor) {
			*ptr = pos;
			pos += askedfor;
			*leftp -= askedfor;
			return (askedfor);
		} else	{
			*ptr = pos;
			count = *leftp;
			*leftp = 0;
			(*ip)++;
			return (count);
		}
	}

	if (*ip == msg->msg_iovlen)
		return (0);

	pos = msg->msg_iov[*ip].iov_base;
	*leftp = msg->msg_iov[*ip].iov_len;

	goto again;
}

/*
 * Copy data to output buffer and align it as in input buffer
 * This is to ensure that if the user wants to align a network
 * addr on a non-word boundry then it will happen.
 */
void
_s_aligned_copy(buf, len, init_offset, datap, rtn_offset)
	register char	*buf;
	register char	*datap;
	register int	*rtn_offset;
	register int	len;
	register int	init_offset;
{
		*rtn_offset = ROUNDUP(init_offset) + ((unsigned int)datap&0x03);
		(void)memcpy((caddr_t)(buf + *rtn_offset), datap, (int)len);
}


/*
 * Max - return max between two ints
 */
int
_s_max(x, y)
	int	x;
	int	y;
{
	if (x > y)
		return (x);
	else	return (y);
}

int
_s_min(x, y)
	int	x;
	int	y;
{
	if (x < y)
		return (x);
	else	return (y);
}


/*
 * Wait for T_OK_ACK
 */
_s_is_ok(siptr, type)
	register struct _si_user	*siptr;
	long				type;
{

	struct strbuf			ctlbuf;
	register union T_primitives	*pptr;
	int				flags;
	int				retval;
	int				fmode;

	fmode = _fcntl(siptr->fd, F_GETFL, 0);
	if (fmode & (O_NDELAY | O_NONBLOCK)) {
		_fcntl(siptr->fd, F_SETFL, fmode & ~O_NDELAY);
	}

	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;
	ctlbuf.maxlen = siptr->ctlsize;
	flags = RS_HIPRI;

	while ((retval = getmsg(siptr->fd, &ctlbuf, NULL, &flags)) < 0) {
		if (errno == EINTR)
			continue;
		return (0);
	}

	/*
	 * Did I get entire message
	 */
	if (retval) {
		errno = EIO;
		return (0);
	}

	/*
	 * Is ctl part large enough to determine type?
	 */
	if (ctlbuf.len < sizeof (long)) {
		errno = EPROTO;
		return (0);
	}

	if (fmode & (O_NDELAY | O_NONBLOCK))
		(void)_fcntl(siptr->fd, F_SETFL, fmode);

	pptr = (union T_primitives *)ctlbuf.buf;
	switch (pptr->type) {
		case T_OK_ACK:
			if ((ctlbuf.len < sizeof (struct T_ok_ack)) ||
			    (pptr->ok_ack.CORRECT_prim != type)) {
				errno = EPROTO;
				return (0);
			}
			return (1);

		case T_ERROR_ACK:
			if ((ctlbuf.len < sizeof (struct T_error_ack)) ||
			    (pptr->error_ack.ERROR_prim != type)) {
				errno = EPROTO;
				return (0);
			}
			if (pptr->error_ack.TLI_error == TSYSERR)
				errno = pptr->error_ack.UNIX_error;
			else	errno = tlitosyserr(pptr->error_ack.TLI_error);
			return (0);

		default:
			errno = EPROTO;
			return (0);
	}
}
/*
 * Translate a TLI error into a system error as best we can.
 */
static const int	tli_errs[] = {
		0,		/* <no error>		*/
		EADDRNOTAVAIL,	/* TBADADDR		*/
		ENOPROTOOPT,	/* TBADOPT		*/
		EACCES,		/* TACCES		*/
		EBADF,		/* TBADF		*/
		EADDRNOTAVAIL,	/* TNOADDR		*/
		EPROTO,		/* TOUTSTATE		*/
		EPROTO,		/* TBADSEQ		*/
		0,		/* TSYSERR 		*/
		EPROTO,		/* TLOOK		*/
		EMSGSIZE,	/* TBADDATA		*/
		EMSGSIZE,	/* TBUFOVFLW		*/
		EPROTO,		/* TFLOW		*/
		EWOULDBLOCK,	/* TNODATA		*/
		EPROTO,		/* TNODIS		*/
		EPROTO,		/* TNOUDERR		*/
		EINVAL,		/* TBADFLAG		*/
		EPROTO,		/* TNOREL		*/
		EOPNOTSUPP,	/* TNOTSUPPORT		*/
		EPROTO,		/* TSTATECHNG		*/
		EPROTO,		/* TNOSTRUCTYPE		*/
		EPROTO,		/* TBADNAME		*/
		EPROTO,		/* TBADQLEN		*/
		EADDRINUSE,	/* TADDRBUSY		*/
		EPROTO,		/* TINDOUT		*/
		EPROTO,		/* TPROVMISMATCH	*/
		EPROTO,		/* TRESQLEN		*/
		EPROTO,		/* TRESADDR		*/
		EPROTO,		/* TQFULL		*/
		EPROTO,		/* TPROTO		*/
};

static int
tlitosyserr(terr)
	register int	terr;
{
	if (terr >= (sizeof (tli_errs) / sizeof *tli_errs))
		return (EPROTO);
	else
		return (tli_errs[terr]);
}


/*
 * timod ioctl
 */
int
_s_do_ioctl(s, buf, size, cmd, retlen)
	register char		*buf;
	register int		*retlen;
{
	register int		retval;
	struct strioctl		strioc;

	strioc.ic_cmd = cmd;
	strioc.ic_timout = -1;
	strioc.ic_len = size;
	strioc.ic_dp = buf;

	if ((retval = _ioctl(s, I_STR, &strioc)) < 0) {
		/*
		 * Map the errno as appropriate.
		 */
		switch (errno) {
			case ENOTTY:
			case ENODEV:
				errno = ENOTSOCK;
				break;

			case EBADF:
				break;

			case ENXIO:
				errno = EPIPE;

			default:
				break;
		}
		return (0);
	}

	if (retval) {
		if ((retval & 0xff) == TSYSERR)
			errno = (retval >>  8) & 0xff;
		else	{
			errno = tlitosyserr(retval & 0xff);
		}
		return (0);
	}
	if (retlen)
		*retlen = strioc.ic_len;
	return (1);
}

/*
 * Allocate buffers
 */
static int
_s_alloc_bufs(siptr, udata)
	register struct _si_user	*siptr;
	register struct si_udata	*udata;
{
	unsigned			size2;

	size2 = sizeof (union T_primitives) + udata->addrsize + sizeof (long) +
			udata->optsize + sizeof (long);


	if ((siptr->ctlsize !=size2) || ( siptr->ctlbuf == (char *)NULL))
	{
		if ((siptr->ctlsize !=0) && (siptr->ctlbuf != (char *)NULL)) 
			(void)free(siptr->ctlbuf);

		if ((siptr->ctlbuf = calloc(1, size2)) == NULL) {
			errno = ENOMEM;
			return (-1);
		}

		siptr->ctlsize = size2;
	}

	return (0);
}

void
_s_close(siptr)
	register struct _si_user	*siptr;

{
	register int			fd;

	fd = siptr->fd;
	(void)free(siptr->ctlbuf);
	(void)delete_silink(fd);
	(void)close(fd);
}

/*
 * Socket interface user structure manipulation routines.
 *
 * A growable array of pointers is allocated.
 * In the multithreaded case, 
 *    the _s_si_user_lock is held for reading while pointers
 *       are being read AND written;
 *    the _s_si_user_lock is held for writing while the size
 *       of the array is changing.  
 * Multithreaded applications must synchronize their access to
 * socket descriptors to avoid multiple allocation of _si_user
 * structures for the same socket descriptor.
 */

/*
 * Allocates a new link and
 * returns a pointer to it
 */
static struct _si_user *
add_silink(s)
	register int 			s;
{
	static struct rlimit		rl;

	int				new_nsd;
	struct _si_user			**new_sdp;
	struct _si_user			**p;
	struct _si_user			*siptr;
	int				i;
	
	if ( s < 0 )
		return (NULL);

	RW_RDLOCK(&_s_si_user_lock);
	/* Has an array of pointers been allocated? */
	if (sdp == NULL) {
		/* No, so release read lock and get write lock. */
		RW_UNLOCK(&_s_si_user_lock);
		RW_WRLOCK(&_s_si_user_lock);
		/* Has the array been allocated yet? */
		if (sdp == NULL) {
			/* No.  Do so now. */
			if (getrlimit(RLIMIT_NOFILE, &rl) != 0) {
				RW_UNLOCK(&_s_si_user_lock);
				return (NULL);
			}
			nsd = _s_min(_s_max(s+DEFAULT_MIN_SDS,
					    DEFAULT_MIN_SDS), rl.rlim_max);
			if ((sdp = (struct _si_user **)
				   calloc(nsd, sizeof (siptr))) == NULL) {
				RW_UNLOCK(&_s_si_user_lock);
				return (NULL);
			}
		} else {
			/* Is the current array big enough for s? */
			if (s > nsd-1) {
				/* No, so increase the array. */
				new_nsd = _s_min(_s_max(s+DEFAULT_MIN_SDS,
							nsd+DEFAULT_MIN_SDS),
						 rl.rlim_max);
				/* Did the array increase in size? */
				if (new_nsd == nsd) {
					/* No, so give up. */
					RW_UNLOCK(&_s_si_user_lock);
					return (NULL);
				}
				/* Grow the array of pointers */
				if ((new_sdp
				     = realloc(sdp, new_nsd * sizeof (siptr)))
				    == NULL) {
					/* 
					 * Failed, so give up, leaving
					 * sdp and nsd as they were.
					 */
					RW_UNLOCK(&_s_si_user_lock);
					return (NULL);
				}
				/* 
				 * Success, so update nsd and sdp and
				 * initialize the new pointers.
				 */
				sdp = new_sdp;
				for (i=1,p=sdp+nsd; i<=(new_nsd-nsd); i++,p++)
					*p = (struct _si_user *)NULL;
				nsd = new_nsd;
			}
		}
	} else {
		/* Is the current array big enough for s? */
		if (s > nsd-1) {
			/* No, so get a write lock and double the array. */
			RW_UNLOCK(&_s_si_user_lock);
			RW_WRLOCK(&_s_si_user_lock);
			if (s > nsd-1) {
				new_nsd = _s_min(_s_max(s+DEFAULT_MIN_SDS,
							nsd+DEFAULT_MIN_SDS),
						 rl.rlim_max);
				/* Did the array increase in size? */
				if (new_nsd == nsd) {
					/* No, so give up. */
					RW_UNLOCK(&_s_si_user_lock);
					return (NULL);
				}
				/* Grow the array of pointers */
				if ((new_sdp
			    	     = realloc(sdp, new_nsd * sizeof (siptr)))
			    	    == NULL) {
					/* 
				 	* Failed, so give up, leaving
				 	* sdp and nsd as they were.
				 	*/
					RW_UNLOCK(&_s_si_user_lock);
					return (NULL);
				}
				/* 
			 	 * Success, so update nsd and sdp and
			 	 * initialize the new pointers.
			 	 */
				sdp = new_sdp;
				for (i=1,p=sdp+nsd; i<=(new_nsd-nsd); i++,p++)
					*p = (struct _si_user *)NULL;
				nsd = new_nsd;
			}
		}
	}

	/* 
	 * There is a pointer for an _si_user structure for s.
	 * Rely on the application to initialize it only once.
	 */

	if (sdp[s] == NULL) 
		sdp[s] = calloc (1, sizeof(struct _si_user));
	siptr = sdp[s];
	siptr->fd = s;

	RW_UNLOCK(&_s_si_user_lock);
	return (siptr);
}

/*
 * Find a link by descriptor
 */
static struct _si_user *
find_silink(s)
	register int	s;
{
	struct _si_user	*siptr;

	if ( s < 0 || s >= nsd ) {
		errno = EBADF;
		return (NULL);
	}

	RW_RDLOCK(&_s_si_user_lock);
	siptr = sdp[s];
	RW_UNLOCK(&_s_si_user_lock);

	if (siptr == NULL || siptr->fd != s) {
		errno = EINVAL;
		return (NULL);
	}
	return (siptr);
}

static int
delete_silink(s)
	register int	s;
{
	if (s < 0 || s >= nsd) {
		errno = EBADF;
		return (-1);
	}

	RW_RDLOCK(&_s_si_user_lock);
	if (sdp[s] == NULL) {
		RW_UNLOCK(&_s_si_user_lock);
		errno = EINVAL;
		return (-1);
	}
	free(sdp[s]);
	sdp[s] = NULL;
	RW_UNLOCK(&_s_si_user_lock);
	return (0);
}

/*
 * Returns the protocol family associated
 * with an endpoint.
 */
int
_s_getfamily(siptr)
	register struct _si_user	*siptr;
{
	register int			i;
	struct stat			statd;
	dev_t				needev;
	register struct netconfig	*net;
	void				*nethandle;

	if (siptr->family != -1)
		return (siptr->family);

	/*
	 * Look up the netconfig structure to
	 * determine the address family.
	 */
	if (fstat(siptr->fd, &statd) < 0)
		return (-1);
	needev = major(statd.st_rdev);

	/*
	 * Loop through each entry in netconfig
	 * until one matches.
	 */
	if ((nethandle = setnetconfig()) == NULL) {
		int save_errno;

		save_errno = errno;
		syslog(LOG_ERR,
		    gettxt("uxnsl:178",
			"_s_getfamily: setnetconfig failed"));
		errno = save_errno;
		return (-1);
	}
	while ((net = getnetconfig(nethandle)) != NULL) {
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
	 * Now, we have to convert from the string
	 * representation as enjoyed by the network
	 * selection stuff, to the integer
	 * representation that the rest of the
	 * world know and love.
	 */
	for (i = 0; family_sw[i] != NULL; i++)
		if (strcmp(family_sw[i], net->nc_protofmly) == 0)
			break;

	endnetconfig(nethandle);
	siptr->family = i;
	return (i);
}

/*
 * Return the number of bytes in the UNIX
 * pathname, not including the null terminator
 * (if any).
 */
int
_s_uxpathlen(un)
	register struct sockaddr_un	*un;
{
	register int			i;

	for (i = 0; i < sizeof (un->sun_path); i++)
		if (un->sun_path[i] == NULL)
			return (i);
	return (sizeof (un->sun_path));
}

int
_s_cpaddr(siptr, to, tolen, from, fromlen)
	register struct _si_user	*siptr;
	register char			*to;
	register int			tolen;
	register char			*from;
	register int			fromlen;

{
	(void)memset(to, 0, tolen);
	if (_s_getfamily(siptr) == AF_INET) {
		if (tolen > sizeof (struct sockaddr_in))
			tolen = sizeof (struct sockaddr_in);
	} else	if (tolen > fromlen)
			tolen = fromlen;
	(void)memcpy(to, from, _s_min(fromlen, tolen));
	return (tolen);
}

/*
 * Common routine to get socket name or connected peer name
 * depending on the value of "flag"
 */
int
_s_getname(siptr, name, namelen, flag)
	struct _si_user		*siptr;
	struct sockaddr		*name;
	int			*namelen;
	int			 flag;
{
	struct T_addr_req	 areq;
	struct T_addr_ack	*aackp;
	struct strbuf		 ctlbuf;
	int			 band = 0;
	int 			 flags = MSG_HIPRI;
	int			 fctlflg;
	int			 blocking;
	int			 i;
	const int		 ntries = 10;
	sigset_t		 oldmask;
	sigset_t		 newmask;

	if ((fctlflg = _fcntl(siptr->fd, F_GETFL, 0)) < 0)
		return (-1);
	blocking = !(fctlflg & (O_NDELAY | O_NONBLOCK));

	areq.PRIM_type = T_ADDR_REQ;
	ctlbuf.len = sizeof(struct T_addr_req);
	ctlbuf.buf = (caddr_t)&areq;

	(void) sigemptyset(&newmask);
	(void) sigaddset(&newmask, SIGPOLL);
	(void) sigprocmask(SIG_BLOCK, &newmask, &oldmask);

	if (putmsg(siptr->fd, &ctlbuf, NULL, 0) < 0) {
		switch(errno) {
		case ENOSTR:
		case ENODEV:
			errno = ENOTSOCK;
			break;
		case ENXIO:
			errno = 0;
			break;
		default:
			break;
		}
		(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
		return (-1);
	}

	ctlbuf.maxlen = siptr->ctlsize;
	ctlbuf.len = 0;
	ctlbuf.buf = siptr->ctlbuf;

	/*
	 * This loop is used to deal with the asynchronous relationship
	 * between the sockets library and the STREAMS mechanism.
	 * Occasionally the call to getpmsg() results in EAGAIN, 
	 * indicating that the operation should be retried.
	 * For non-blocking sockets the library will try a few times
	 * to get the message, then give up, assuming something
	 * is actually wrong.
	 * To increase the chances of success, a sleep(1) is used
	 * between calls to getpmsg().
	 */

	for (i=0; i<ntries; i++) {
		if (getpmsg(siptr->fd, &ctlbuf, NULL, &band, &flags) < 0) {
			if (blocking || errno != EAGAIN) {
				(void) sigprocmask(SIG_SETMASK, &oldmask, 
						   (sigset_t *)NULL);
				return (-1);
			}
			/* EAGAIN caused the failure of getpmsg() */
			sleep(1);
			continue;
		}
		/* getpmsg() succeeded, so leave loop with i<ntries. */
		break;
	}
	(void) sigprocmask(SIG_SETMASK, &oldmask, (sigset_t *)NULL);
	if (i >= ntries) {
		/* Too many failures, so give up. */
		return (-1);
	}
	/* getpmsg() succeeded. */

	if (ctlbuf.len < sizeof(struct T_addr_ack)) {
		errno = EINVAL;
		return (-1);
	}

	aackp = (struct T_addr_ack *) ctlbuf.buf;
	if (aackp->PRIM_type != T_ADDR_ACK) {
		errno = EINVAL;
		return (-1);
	}

	errno = 0;
	switch (flag) {
	case LOCALNAME:
		*namelen = _s_cpaddr(siptr, name, *namelen, 
				((char *)aackp + aackp->LOCADDR_offset),
				(int)aackp->LOCADDR_length);
		break;
	case REMOTENAME:
		if (aackp->REMADDR_length == 0) {
			errno = ENOTCONN;
			return (-1);
		}
		*namelen = _s_cpaddr(siptr, name, *namelen,
				((char *)aackp + aackp->REMADDR_offset),
				(int)aackp->REMADDR_length);
		break;
	}
	return (0);
}

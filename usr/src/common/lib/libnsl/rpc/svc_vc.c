/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/svc_vc.c	1.8.11.6"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * svc_vc.c, Server side for Connection Oriented RPC.
 *
 * Actually implements two flavors of transporter -
 * a rendezvouser (a listener and connection establisher)
 * and a record stream.
 */

#include <stdio.h>
#include <unistd.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include "trace.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/mkdev.h>
#include <sys/poll.h>
#include <sys/syslog.h>
#include <rpc/nettype.h>
#include <xti.h>

#ifndef MIN
#define	MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif

extern bool_t	abort();

static struct xp_ops 	*svc_vc_ops();
static struct xp_ops 	*svc_vc_rendezvous_ops();
static void		svc_vc_destroy();
static int 		read_vc();
static int 		write_vc();
static SVCXPRT 		*makefd_xprt();
extern char 		*strdup(), *malloc();

struct cf_rendezvous { /* kept in xprt->xp_p1 for rendezvouser */
	u_int sendsize;
	u_int recvsize;
	struct t_call *t_call;
	struct t_bind *t_bind;
	long cf_tsdu;
};

struct cf_conn {	/* kept in xprt->xp_p1 for actual connection */
	enum xprt_stat strm_stat;
	u_long x_id;
	long cf_tsdu;
	XDR xdrs;
	char verf_body[MAX_AUTH_BYTES];
};

static int t_rcvall();

/*
 * Usage:
 *	xprt = svc_vc_create(fd, sendsize, recvsize);
 * Since connection streams do buffered io similar to stdio, the caller
 * can specify how big the send and receive buffers are. If recvsize
 * or sendsize are 0, defaults will be chosen.
 * fd should be open and bound.
 */
SVCXPRT *
svc_vc_create(fd, sendsize, recvsize)
	register int fd;
	u_int sendsize;
	u_int recvsize;
{
	register struct cf_rendezvous *r;
	SVCXPRT *xprt;
	struct t_info tinfo;

	trace4(TR_svc_vc_create, 0, fd, sendsize, recvsize);
	xprt = (SVCXPRT *)mem_alloc(sizeof (SVCXPRT));
	if (xprt == (SVCXPRT *)NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32", "%s: out of memory"),
		    "svc_vc_create");
		trace2(TR_svc_vc_create, 1, fd);
		return ((SVCXPRT *)NULL);
	}
	memset((char *)xprt, 0, sizeof (SVCXPRT));

	r = (struct cf_rendezvous *)mem_alloc(sizeof (*r));
	if (r == (struct cf_rendezvous *)NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32", "%s: out of memory"),
		    "svc_vc_create");
		(void) mem_free((void *)xprt, sizeof (SVCXPRT));
		trace2(TR_svc_vc_create, 1, fd);
		return ((SVCXPRT *)NULL);
	}
	if (t_getinfo(fd, &tinfo) == -1) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:124",
			"svc_vc_create: could not get transport information"));
		(void) mem_free((void *)xprt, sizeof (SVCXPRT));
		(void) mem_free((void *)r, sizeof (r*));
		trace2(TR_svc_vc_create, 1, fd);
		return ((SVCXPRT *)NULL);
	}
	/*
	 * Find the receive and the send size
	 */
	r->sendsize = _rpc_get_t_size((int)sendsize, tinfo.tsdu);
	r->recvsize = _rpc_get_t_size((int)recvsize, tinfo.tsdu);
	if ((r->sendsize == 0) || (r->recvsize == 0)) {
		syslog(LOG_ERR,
		  gettxt("uxnsl:125",
		   "svc_vc_create:  transport does not support data transfer"));
		(void) mem_free((void *)xprt, sizeof (SVCXPRT));
		(void) mem_free((void *)r, sizeof (r*));
		trace2(TR_svc_vc_create, 1, fd);
		return ((SVCXPRT *)NULL);
	}

	r->t_call = (struct t_call *) t_alloc(fd, T_CALL, T_ADDR | T_OPT);
	if (r->t_call == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32", "%s: out of memory"),
		    "svc_vc_create");
		(void) mem_free((void *)xprt, sizeof (SVCXPRT));
		(void) mem_free((void *)r, sizeof (r*));
		return ((SVCXPRT *)NULL);
	}

	r->t_bind = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR);
	if (r->t_bind == (struct t_bind *) NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32", "%s: out of memory"),
		    "svc_vc_create");
		(void) t_free((char *)r->t_call, T_CALL);
		(void) mem_free((void *)xprt, sizeof (SVCXPRT));
		(void) mem_free((void *)r, sizeof (r*));
		return ((SVCXPRT *)NULL);
	}


	r->cf_tsdu = tinfo.tsdu;
	xprt->xp_fd = fd;
	xprt->xp_port = -1;	/* It is the rendezvouser */
	xprt->xp_p1 = (caddr_t)r;
	xprt->xp_p2 = NULL;
	xprt->xp_p3 = NULL;
	xprt->xp_verf = _null_auth;
	xprt->xp_ops = svc_vc_rendezvous_ops();
	xprt_register(xprt);
	trace2(TR_svc_vc_create, 1, fd);
	return (xprt);
}

/*
 * used for the actual connection.
 */
SVCXPRT *
svc_fd_create(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	struct t_info tinfo;
	SVCXPRT *dummy;

	trace4(TR_svc_fd_create, 0, fd, sendsize, recvsize);
	if (t_getinfo(fd, &tinfo) == -1) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:126",
			"svc_fd_create: could not get transport information"));
		trace2(TR_svc_fd_create, 1, fd);
		return ((SVCXPRT *)NULL);
	}
	/*
	 * Find the receive and the send size
	 */
	sendsize = _rpc_get_t_size((int)sendsize, tinfo.tsdu);
	recvsize = _rpc_get_t_size((int)recvsize, tinfo.tsdu);
	if ((sendsize == 0) || (recvsize == 0)) {
		syslog(LOG_ERR,
		  gettxt("uxnsl:127",
		    "svc_fd_create: transport does not support data transfer"));
		trace2(TR_svc_fd_create, 1, fd);
		return ((SVCXPRT *)NULL);
	}
	dummy = makefd_xprt(fd, sendsize, recvsize, tinfo.tsdu);
	trace2(TR_svc_fd_create, 1, fd);
	return (dummy);
}

static SVCXPRT *
makefd_xprt(fd, sendsize, recvsize, tsdu)
	int fd;
	u_int sendsize;
	u_int recvsize;
	long tsdu;
{
	register SVCXPRT *xprt;
	register struct cf_conn *cd;

	trace5(TR_makefd_xprt, 0, fd, sendsize, recvsize, tsdu);
	xprt = (SVCXPRT *)mem_alloc(sizeof (SVCXPRT));
	if (xprt == (SVCXPRT *)NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32", "%s: out of memory"),
		    "makefd_xprt");
		trace2(TR_makefd_xprt, 1, fd);

		return ((SVCXPRT *)NULL);
	}
	(void) memset((char *)xprt, 0, sizeof (SVCXPRT));
	cd = (struct cf_conn *)mem_alloc(sizeof (struct cf_conn));
	if (cd == (struct cf_conn *)NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32", "%s: out of memory"),
		    "makefd_xprt");
		(void) mem_free((char *) xprt, sizeof (SVCXPRT));
		trace2(TR_makefd_xprt, 1, fd);
		return ((SVCXPRT *)NULL);
	}
	cd->strm_stat = XPRT_IDLE;
	cd->cf_tsdu = tsdu;
	xdrrec_create(&(cd->xdrs), sendsize, recvsize, (caddr_t)xprt,
			read_vc, write_vc);
	xprt->xp_p1 = (caddr_t)cd;
	xprt->xp_p2 = NULL;
	xprt->xp_verf.oa_base = cd->verf_body;
	xprt->xp_ops = svc_vc_ops();	/* truely deals with calls */
	xprt->xp_port = 0;	/* this is a connection, not a rendezvouser */
	xprt->xp_fd = fd;
	xprt_register(xprt);
	trace2(TR_makefd_xprt, 1, fd);
	return (xprt);
}

/*
 * This routine is called by svc_getreqset(), when a packet is recd.
 * The listener process creates another end point on which the actual
 * connection is carried. It returns FALSE to indicate that it was
 * not a rpc packet (falsely though), but as a side effect creates
 * another endpoint which is also registered, which then always
 * has a request ready to be served.
 */
/* ARGSUSED1 */
static bool_t
rendezvous_request(xprt, msg)
	register SVCXPRT *xprt;
	struct rpc_msg *msg; /* needed for ANSI-C typechecker */
{
	register int fd = RPC_ANYFD;
	struct cf_rendezvous *r;
	struct t_call *t_call, t_call2;
	struct t_info tinfo;
	char *tpname = NULL;
	char devbuf[256];
	static void do_accept();

	trace1(TR_rendezvous_request, 0);
	r = (struct cf_rendezvous *) xprt->xp_p1;

again:
	switch (t_look(xprt->xp_fd)) {
	case T_DISCONNECT:
		(void) t_rcvdis(xprt->xp_fd, NULL);
		trace1(TR_rendezvous_request, 1);
		return (FALSE);

	case T_LISTEN:

		if (t_listen(xprt->xp_fd, r->t_call) == -1) {
			if ((t_errno == TSYSERR) && (errno == EINTR))
				goto again;

			if (t_errno == TLOOK) {
				if (t_look(xprt->xp_fd) == T_DISCONNECT)
				    (void) t_rcvdis(xprt->xp_fd, NULL);
			}
			trace1(TR_rendezvous_request, 1);
			return (FALSE);
		}
		break;
	default:
		trace1(TR_rendezvous_request, 1);
		return (FALSE);
	}
	/*
	 * Now create another endpoint, and accept the connection
	 * on it.
	 */

	if (xprt->xp_tp) {
		tpname = xprt->xp_tp;
	} else {
		/*
		 * If xprt->xp_tp is NULL, then try all
		 * possible connection oriented transports until
		 * one succeeds in finding an appropriate one.
		 * This code is almost identical to that in
		 * accept for libsocket
		 */

		struct stat statbuf;
		void *hndl;
		struct netconfig *nconf;
		dev_t devno;

		if (fstat(xprt->xp_fd, &statbuf) == -1) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:128",
			       "rendezvous_request: can't find device number"));
			goto err;
		}

		devno = major(statbuf.st_rdev);

		hndl = setnetconfig();
		if (hndl == NULL) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:129",
			 "rendezvous_request: cannot read netconfig database"));
			goto err;
		}
		tpname = devbuf;
		while (nconf = getnetconfig(hndl)) {
			if ((nconf->nc_semantics != NC_TPI_COTS) &&
				(nconf->nc_semantics != NC_TPI_COTS_ORD))
				continue;
			if (!stat(nconf->nc_device, &statbuf) &&
				(devno == minor(statbuf.st_rdev))) {
				strcpy(tpname, nconf->nc_device);
				break;
			}
		}
		endnetconfig(hndl);
		if (!nconf) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:130",
				"rendezvous_request: no suitable transport"));
			goto err;
		}
	}

	do_accept(xprt->xp_fd, tpname, xprt->xp_netid, r->t_call, r);

err:
	trace1(TR_rendezvous_request, 1);
	return (FALSE); /* there is never an rpc msg to be processed */
}

static void
do_accept(srcfd, tpname, netid, tcp, r)
int	srcfd;
char	*tpname, *netid;
struct t_call	*tcp;
struct cf_rendezvous	*r;
{
	int	destfd;
	struct t_call	t_call;
	struct t_call	*tcp2 = (struct t_call *) NULL;
	struct t_info	tinfo;
	SVCXPRT	*xprt = (SVCXPRT *) NULL;

	trace1(TR_do_accept, 0);
	destfd = t_open(tpname, O_RDWR, &tinfo);
	if (destfd == -1) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:131",
			"do_accept: can't open connection: %s"),
		    t_strerror(t_errno));
		trace1(TR_do_accept, 1);
		return;
	}
	if ((tinfo.servtype != T_COTS) && (tinfo.servtype != T_COTS_ORD)) {
		/* Not a connection oriented mode */
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:132", "do_accept:  illegal transport"));
		(void) t_close(destfd);
		trace1(TR_do_accept, 1);
		return;
	}

	if (t_bind(destfd, (struct t_bind *) NULL, r->t_bind) == -1) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:133", "do_accept: t_bind failed: %s"),
		    t_strerror(t_errno));
		(void) t_free((char *) r->t_bind, T_BIND);
		(void) t_close(destfd);
		trace1(TR_do_accept, 1);
		return;
	}
	/*
	 * This connection is not listening, hence no need to set
	 * the qlen.
	 */

	/*
	 * XXX: The local transport chokes on its own listen
	 * options so we zero them for now
	 */
	t_call = *tcp;
	t_call.opt.len = 0;
	t_call.opt.maxlen = 0;
	t_call.opt.buf = (char *) NULL;

	while (t_accept(srcfd, destfd, &t_call) == -1) {
		switch (t_errno) {
		case TLOOK:
again:
			switch (t_look(srcfd)) {
			case T_CONNECT:
			case T_DATA:
			case T_EXDATA:
				/* this should not happen */
				break;

			case T_DISCONNECT:
				(void) t_rcvdis(srcfd,
					(struct t_discon *) NULL);
				break;

			case T_LISTEN:
				if (tcp2 == (struct t_call *) NULL)
					tcp2 = (struct t_call *) t_alloc(srcfd,
					    T_CALL, T_ADDR | T_OPT);
				if (tcp2 == (struct t_call *) NULL) {

					(void) t_close(destfd);
					trace1(TR_do_accept, 1);
					return;
				}
				if (t_listen(srcfd, tcp2) == -1) {
					switch (t_errno) {
					case TSYSERR:
						if (errno == EINTR)
							goto again;
						break;

					case TLOOK:
						goto again;
					}
					(void) t_free((char *)tcp2, T_CALL);
					(void) t_close(destfd);
					trace1(TR_do_accept, 1);
					return;
					/* NOTREACHED */
				}

				do_accept(srcfd, tpname, netid, tcp2, r);
				(void) t_free((char *) tcp2, T_CALL);
				tcp2 = (struct t_call *) NULL;
				break;

			case T_ORDREL:
				(void) t_rcvrel(srcfd);
				(void) t_sndrel(srcfd);
				break;
			}
			break;

		case TOUTSTATE:
			/*
			 *	This can happen if the t_rcvdis() or t_rcvrel()/
			 *	t_sndrel() put srcfd into the T_IDLE state.
			 */
			if (t_getstate(srcfd) == T_IDLE) {
				(void) t_free((char *) r->t_bind, T_BIND);
				(void) t_close(destfd);
				trace1(TR_do_accept, 1);
				return;
			}
			/* else FALL THROUGH TO */

		default:
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:134",
		 "do_accept: cannot accept connection:  %s (current state %d)"),
			    t_strerror(t_errno), t_getstate(srcfd));
			(void) t_close(destfd);
			trace1(TR_do_accept, 1);
			return;
			/* NOTREACHED */
		}
	}

	/*
	 * make a new transporter
	 */
	xprt = makefd_xprt(destfd, r->sendsize, r->recvsize, r->cf_tsdu);
	if (xprt == (SVCXPRT *) NULL) {
		(void) t_close(destfd);
		trace1(TR_do_accept, 1);
		return;
	}

	/*
	 * Copy the new local and remote bind information
	 */

	xprt->xp_rtaddr.len = tcp->addr.len;
	xprt->xp_rtaddr.maxlen = tcp->addr.len;
	xprt->xp_rtaddr.buf = malloc(tcp->addr.len);
	memcpy(xprt->xp_rtaddr.buf, tcp->addr.buf, tcp->addr.len);

	xprt->xp_tp = strdup(tpname);
	xprt->xp_netid = strdup(netid);
	if ((xprt->xp_tp == (char *) NULL) ||
	    (xprt->xp_netid == (char *) NULL)) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32", "%s: out of memory"),
		    "do_accept");
		if (xprt)
			svc_vc_destroy(xprt);
		(void) t_close(destfd);
		trace1(TR_do_accept, 1);
		return;
	}
	if (tcp->opt.len > 0) {
		struct netbuf *netptr;

		xprt->xp_p2 = malloc(sizeof (struct netbuf));

		if (xprt->xp_p2 != (char *) NULL) {
			netptr = (struct netbuf *) xprt->xp_p2;

			netptr->len = tcp->opt.len;
			netptr->maxlen = tcp->opt.len;
			netptr->buf = malloc(tcp->opt.len);
			memcpy(netptr->buf, tcp->opt.buf, tcp->opt.len);
		}
	}
}

/* ARGSUSED */
static enum xprt_stat
rendezvous_stat(xprt)
	SVCXPRT *xprt; /* needed for ANSI-C typechecker */
{
	trace1(TR_rendezvous_stat, 0);
	trace1(TR_rendezvous_stat, 1);
	return (XPRT_IDLE);
}

static void
svc_vc_destroy(xprt)
	register SVCXPRT *xprt;
{
	register struct cf_conn *cd = (struct cf_conn *) xprt->xp_p1;
	register struct cf_rendezvous *r;

	struct pollfd pfd;

	trace1(TR_svc_vc_destroy, 0);
	xprt_unregister(xprt);
	if (xprt->xp_type == T_COTS_ORD)
		(void) t_sndrel(xprt->xp_fd);
	/*
	 * XXX
	 * We should really do a t_rcvrel() at this point.
	 * However, we dont want to block on a t_rcvrel()
	 * Doing a non-blocking fd involves polling continually.
	 * So, for now just do a t_close().
	 */
	t_close(xprt->xp_fd);

	if (xprt->xp_port != 0) {
		/* a rendezvouser end point */
		xprt->xp_port = 0;
		r = (struct cf_rendezvous *) xprt->xp_p1;
		t_free((char *) r->t_call, T_CALL);
		t_free((char *) r->t_bind, T_BIND);
	} else {

		XDR_DESTROY(&(cd->xdrs));
	}
	(void) mem_free((caddr_t)cd, sizeof (*cd));
	if (xprt->xp_rtaddr.buf)
		(void) mem_free(xprt->xp_rtaddr.buf, xprt->xp_rtaddr.maxlen);
	if (xprt->xp_ltaddr.buf)
		(void) mem_free(xprt->xp_ltaddr.buf, xprt->xp_ltaddr.maxlen);
	if (xprt->xp_tp)
		(void) free(xprt->xp_tp);
	if (xprt->xp_netid)
		(void) free(xprt->xp_netid);
	if (xprt->xp_p2) {
		(void) mem_free((caddr_t)((struct netbuf *) xprt->xp_p2)->buf,
			((struct netbuf *) xprt->xp_p2)->len);
		(void) mem_free((struct netbuf *) xprt->xp_p2,
			sizeof (struct netbuf));
	}

	(void) mem_free((caddr_t)xprt, sizeof (SVCXPRT));
	trace1(TR_svc_vc_destroy, 1);
}

/*
 * All read operations timeout after 35 seconds.
 * A timeout is fatal for the connection.
 */
#define	WAIT_PER_TRY	35000	/* milliseconds */

/*
 * reads data from the vc conection.
 * any error is fatal and the connection is closed.
 * (And a read of zero bytes is a half closed stream => error.)
 */
static int
read_vc(xprt, buf, len)
	register SVCXPRT *xprt;
	caddr_t buf;
	register int len;
{
	register int fd = xprt->xp_fd;
	struct pollfd pfd;

	trace2(TR_read_vc, 0, len);
	pfd.fd = fd;
	pfd.events = POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND;

	do {
		if (poll(&pfd, 1, WAIT_PER_TRY) <= 0)
			goto fatal_err;
	} while (pfd.revents == 0);
	if (pfd.revents & POLLNVAL)
		goto fatal_err;
	if ((len = t_rcvall(fd, buf, len)) > 0) {
		trace1(TR_read_vc, 1);
		return (len);
	}
fatal_err:
	((struct cf_conn *)(xprt->xp_p1))->strm_stat = XPRT_DIED;
	trace1(TR_read_vc, 1);
	return (-1);
}

/*
 * Receive the required bytes of data, even if it is fragmented.
 */
static int
t_rcvall(fd, buf, len)
	int fd;
	char *buf;
	int len;
{
	int flag;
	int final = 0;
	int res;

	trace3(TR_t_rcvall, 0, fd, len);
	do {
		res = t_rcv(fd, buf, (unsigned)len, &flag);
		if (res == -1) {
			if (t_errno == TLOOK) {
				switch (t_look(fd)) {
				case T_DISCONNECT:
					t_rcvdis(fd, NULL);
					break;
				case T_ORDREL:
					t_rcvrel(fd);
					(void) t_sndrel(fd);
					break;
				default:
					break;
				}
			}
			break;
		}
		final += res;
		buf += res;
		len -= res;
	} while (len && (flag & T_MORE));
	trace2(TR_t_rcvall, 1, fd);
	return (res == -1 ? -1 : final);
}

/*
 * writes data to the vc connection.
 * Any error is fatal and the connection is closed.
 */
static int
write_vc(xprt, buf, len)
	register SVCXPRT *xprt;
	caddr_t buf;
	int len;
{
	register int i, cnt;
	int flag;
	long maxsz;

	trace2(TR_write_vc, 0, len);
	maxsz = ((struct cf_conn *)(xprt->xp_p1))->cf_tsdu;
	if ((maxsz == 0) || (maxsz == -1)) {
		if ((len = t_snd(xprt->xp_fd, buf, (unsigned)len,
				(int)0)) == -1) {
			if (t_errno == TLOOK) {
				switch (t_look(xprt->xp_fd)) {
				case T_DISCONNECT:
					t_rcvdis(xprt->xp_fd, NULL);
					break;
				case T_ORDREL:
					t_rcvrel(xprt->xp_fd);
					(void) t_sndrel(xprt->xp_fd);
					break;
				default:
					break;
				}
			}
			((struct cf_conn *)(xprt->xp_p1))->strm_stat
					= XPRT_DIED;
		}
		trace1(TR_write_vc, 1);
		return (len);
	}

	/*
	 * This for those transports which have a max size for data.
	 */
	for (cnt = len; cnt > 0; cnt -= i, buf += i) {
		flag = cnt > maxsz ? T_MORE : 0;
		if ((i = t_snd(xprt->xp_fd, buf,
			(unsigned)MIN(cnt, maxsz), flag)) == -1) {
			if (t_errno == TLOOK) {
				switch (t_look(xprt->xp_fd)) {
				case T_DISCONNECT:
					t_rcvdis(xprt->xp_fd, NULL);
					break;
				case T_ORDREL:
					t_rcvrel(xprt->xp_fd);
					break;
				default:
					break;
				}
			}
			((struct cf_conn *)(xprt->xp_p1))->strm_stat
					= XPRT_DIED;
			trace1(TR_write_vc, 1);
			return (-1);
		}
	}
	trace1(TR_write_vc, 1);
	return (len);
}

static enum xprt_stat
svc_vc_stat(xprt)
	SVCXPRT *xprt;
{
	register struct cf_conn *cd = (struct cf_conn *)(xprt->xp_p1);

	trace1(TR_svc_vc_stat, 0);
	if (cd->strm_stat == XPRT_DIED) {
		trace1(TR_svc_vc_stat, 1);
		return (XPRT_DIED);
	}
	if (! xdrrec_eof(&(cd->xdrs))) {
		trace1(TR_svc_vc_stat, 1);
		return (XPRT_MOREREQS);
	}
	trace1(TR_svc_vc_stat, 1);
	return (XPRT_IDLE);
}

static bool_t
svc_vc_recv(xprt, msg)
	SVCXPRT *xprt;
	register struct rpc_msg *msg;
{
	register struct cf_conn *cd = (struct cf_conn *)(xprt->xp_p1);
	register XDR *xdrs = &(cd->xdrs);

	trace1(TR_svc_vc_recv, 0);
	xdrs->x_op = XDR_DECODE;
	(void) xdrrec_skiprecord(xdrs);
	if (xdr_callmsg(xdrs, msg)) {
		cd->x_id = msg->rm_xid;
		trace1(TR_svc_vc_recv, 1);
		return (TRUE);
	}
	trace1(TR_svc_vc_recv, 1);
	return (FALSE);
}

static bool_t
svc_vc_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	bool_t dummy1;

	trace1(TR_svc_vc_getargs, 0);
	dummy1 = (*xdr_args)(&(((struct cf_conn *)(xprt->xp_p1))->xdrs),
			args_ptr);
	trace1(TR_svc_vc_getargs, 1);
	return (dummy1);
}

static bool_t
svc_vc_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register XDR *xdrs = &(((struct cf_conn *)(xprt->xp_p1))->xdrs);
	bool_t dummy2;

	trace1(TR_svc_vc_freeargs, 0);
	xdrs->x_op = XDR_FREE;
	dummy2 = (*xdr_args)(xdrs, args_ptr);
	trace1(TR_svc_vc_freeargs, 1);
	return (dummy2);
}

static bool_t
svc_vc_reply(xprt, msg)
	SVCXPRT *xprt;
	register struct rpc_msg *msg;
{
	register struct cf_conn *cd = (struct cf_conn *)(xprt->xp_p1);
	register XDR *xdrs = &(cd->xdrs);
	register bool_t stat;

	trace1(TR_svc_vc_reply, 0);
	xdrs->x_op = XDR_ENCODE;
	msg->rm_xid = cd->x_id;
	stat = xdr_replymsg(xdrs, msg);
	(void) xdrrec_endofrecord(xdrs, TRUE);
	trace1(TR_svc_vc_reply, 1);
	return (stat);
}

static struct xp_ops *
svc_vc_ops()
{
	static struct xp_ops ops;

	trace1(TR_svc_vc_ops, 0);
	if (ops.xp_destroy == NULL) {
		ops.xp_recv = svc_vc_recv;
		ops.xp_stat = svc_vc_stat;
		ops.xp_getargs = svc_vc_getargs;
		ops.xp_reply = svc_vc_reply;
		ops.xp_freeargs = svc_vc_freeargs;
		ops.xp_destroy = svc_vc_destroy;
	}
	trace1(TR_svc_vc_ops, 1);
	return (&ops);
}

static struct xp_ops *
svc_vc_rendezvous_ops()
{
	static struct xp_ops ops;

	trace1(TR_svc_vc_rendezvous_ops, 0);
	if (ops.xp_destroy == NULL) {
		ops.xp_recv = rendezvous_request;
		ops.xp_stat = rendezvous_stat;
		ops.xp_getargs = abort;
		ops.xp_reply = abort;
		ops.xp_freeargs = abort,
		ops.xp_destroy = svc_vc_destroy;
	}
	trace1(TR_svc_vc_rendezvous_ops, 1);
	return (&ops);
}

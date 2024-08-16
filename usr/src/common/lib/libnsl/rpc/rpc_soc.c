/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpc_soc.c	1.4.12.3"
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
 * rpc_soc.c
 *
 * The backward compatibility routines for the earlier implementation
 * of RPC, where the only transports supported were tcp/ip and udp/ip.
 * Based on berkeley socket abstraction, now implemented on the top
 * of TLI/Streams
 */

#ifdef PORTMAP

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "trace.h"
#include <rpc/rpc.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netdir.h>
#include <errno.h>
#include <sys/syslog.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_prot.h>
#include <rpc/nettype.h>
#include "rpc_mt.h"

#undef rpc_createerr	/* Need automatic to give set_rpc_createerr() */

static int bindresvport();

/*
 * A common clnt create routine
 */
static CLIENT *
clnt_com_create(raddr, prog, vers, sockp, sendsz, recvsz, tp)
	register struct sockaddr_in *raddr;
	u_long prog;
	u_long vers;
	int *sockp;
	u_int sendsz;
	u_int recvsz;
	char *tp;
{
	CLIENT *cl;
	int madefd = FALSE;
	int fd = *sockp;
	struct t_info tinfo;
	struct netconfig *nconf;
	int port;
	struct netbuf bindaddr;
	rpc_createerr_t rpc_createerr = { 0 };

	trace5(TR_clnt_com_create, 0, prog, vers, sendsz, recvsz);
	if ((nconf = _rpc_getconfip(tp)) == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_clnt_com_create, 1, prog, vers);
		return ((CLIENT *)NULL);
	}
	if (fd == RPC_ANYSOCK) {
		fd = t_open(nconf->nc_device, O_RDWR, &tinfo);
		if (fd == -1)
			goto syserror;
		madefd = TRUE;
	} else {
		if (t_getinfo(fd, &tinfo) == -1)
			goto syserror;
	}

	if (raddr->sin_port == 0) {
		u_int proto;
		u_short sport;

		proto = strcmp(tp, "udp") == 0 ? IPPROTO_UDP : IPPROTO_TCP;
		sport = pmap_getport(raddr, prog, vers, proto);
		if (sport == 0) {
			goto err;
		}
		raddr->sin_port = htons(sport);
	}

	/* Transform sockaddr_in to netbuf */
	bindaddr.maxlen = bindaddr.len =  _rpc_get_a_size(tinfo.addr);
	bindaddr.buf = (char *)raddr;

	(void) bindresvport(fd, (struct sockaddr_in *)NULL, &port, 0);
	cl = clnt_tli_create(fd, nconf, &bindaddr, prog, vers,
				sendsz, recvsz);
	if (cl) {
		if (madefd == TRUE) {
			/*
			 * The fd should be closed while destroying the handle.
			 */
			(void) CLNT_CONTROL(cl, CLSET_FD_CLOSE, (char *)NULL);
			*sockp = fd;
		}
		(void) freenetconfigent(nconf);
		trace3(TR_clnt_com_create, 1, prog, vers);
		return (cl);
	}
	goto err;

syserror:
	rpc_createerr.cf_stat = RPC_SYSTEMERROR;
	rpc_createerr.cf_error.re_errno = errno;
	rpc_createerr.cf_error.re_terrno = t_errno;
	set_rpc_createerr(rpc_createerr);

err:	if (madefd == TRUE)
		(void) t_close(fd);
	(void) freenetconfigent(nconf);
	trace3(TR_clnt_com_create, 1, prog, vers);
	return ((CLIENT *)NULL);
}

CLIENT *
clntudp_bufcreate(raddr, prog, vers, wait, sockp, sendsz, recvsz)
	register struct sockaddr_in *raddr;
	u_long prog;
	u_long vers;
	struct timeval wait;
	int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *cl;

	trace5(TR_clntudp_bufcreate, 0, prog, vers, sendsz, recvsz);
	cl = clnt_com_create(raddr, prog, vers, sockp, sendsz, recvsz, "udp");
	if (cl == (CLIENT *)NULL) {
		trace3(TR_clntudp_bufcreate, 1, prog, vers);
		return ((CLIENT *)NULL);
	}
	(void) CLNT_CONTROL(cl, CLSET_RETRY_TIMEOUT, (char *)&wait);
	trace3(TR_clntudp_bufcreate, 1, prog, vers);
	return (cl);
}

CLIENT *
clntudp_create(raddr, program, version, wait, sockp)
	struct sockaddr_in *raddr;
	u_long program;
	u_long version;
	struct timeval wait;
	int *sockp;
{
	CLIENT *dummy;

	trace3(TR_clntudp_create, 0, program, version);
	dummy = clntudp_bufcreate(raddr, program, version, wait, sockp,
					UDPMSGSIZE, UDPMSGSIZE);
	trace3(TR_clntudp_create, 1, program, version);
	return (dummy);
}

CLIENT *
clnttcp_create(raddr, prog, vers, sockp, sendsz, recvsz)
	struct sockaddr_in *raddr;
	u_long prog;
	u_long vers;
	register int *sockp;
	u_int sendsz;
	u_int recvsz;
{
	CLIENT *dummy;

	trace5(TR_clnttcp_create, 0, prog, vers, sendsz, recvsz);
	dummy = clnt_com_create(raddr, prog, vers, sockp, sendsz,
			recvsz, "tcp");
	trace3(TR_clnttcp_create, 1, prog, vers);
	return (dummy);
}

CLIENT *
clntraw_create(prog, vers)
	u_long prog;
	u_long vers;
{
	CLIENT *dummy;

	trace3(TR_clntraw_create, 0, prog, vers);
	dummy = clnt_raw_create(prog, vers);
	trace3(TR_clntraw_create, 1, prog, vers);
	return (dummy);
}

/*
 * A common server create routine
 */
static SVCXPRT *
svc_com_create(fd, sendsize, recvsize, netid)
	register int fd;
	u_int sendsize;
	u_int recvsize;
	char *netid;
{
	struct netconfig *nconf;
	SVCXPRT *svc;
	int madefd = FALSE;
	int port;
	int res;

	trace4(TR_svc_com_create, 0, fd, sendsize, recvsize);
	if ((nconf = _rpc_getconfip(netid)) == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:88", "RPC: Could not get %s transport"),
		    netid);
		trace2(TR_svc_com_create, 1, fd);
		return ((SVCXPRT *)NULL);
	}
	if (fd == RPC_ANYSOCK) {
		fd = t_open(nconf->nc_device, O_RDWR, (struct t_info *)NULL);
		if (fd == -1) {
			(void) freenetconfigent(nconf);
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:89",
		 "svc_com_create: could not open connection over %s transport"),
			    netid);
			trace2(TR_svc_com_create, 1, fd);
			return ((SVCXPRT *)NULL);
		}
		madefd = TRUE;
	}

	res = bindresvport(fd, (struct sockaddr_in *)NULL, &port, 8);
	svc = svc_tli_create(fd, nconf, (struct t_bind *)NULL,
				sendsize, recvsize);
	(void) freenetconfigent(nconf);
	if (svc == (SVCXPRT *)NULL) {
		if (madefd)
			(void) t_close(fd);
		trace2(TR_svc_com_create, 1, fd);
		return ((SVCXPRT *)NULL);
	}
	if (res == -1) {
		port = (((struct sockaddr_in *)svc->xp_ltaddr.buf)->sin_port);
	}
	svc->xp_port = ntohs(port);
	trace2(TR_svc_com_create, 1, fd);
	return (svc);
}

SVCXPRT *
svctcp_create(fd, sendsize, recvsize)
	register int fd;
	u_int sendsize;
	u_int recvsize;
{
	SVCXPRT *dummy;

	trace4(TR_svctcp_create, 0, fd, sendsize, recvsize);
	dummy = svc_com_create(fd, sendsize, recvsize, "tcp");
	trace4(TR_svctcp_create, 1, fd, sendsize, recvsize);
	return (dummy);
}

SVCXPRT *
svcudp_bufcreate(fd, sendsz, recvsz)
	register int fd;
	u_int sendsz, recvsz;
{
	SVCXPRT *dummy;

	trace4(TR_svcudp_bufcreate, 0, fd, sendsz, recvsz);
	dummy = svc_com_create(fd, sendsz, recvsz, "udp");
	trace4(TR_svcudp_bufcreate, 1, fd, sendsz, recvsz);
	return (dummy);
}

SVCXPRT *
svcfd_create(fd, sendsize, recvsize)
	int fd;
	u_int sendsize;
	u_int recvsize;
{
	SVCXPRT *dummy;

	trace4(TR_svcfd_create, 0, fd, sendsize, recvsize);
	dummy = svc_fd_create(fd, sendsize, recvsize);
	trace4(TR_svcfd_create, 1, fd, sendsize, recvsize);
	return (dummy);
}


SVCXPRT *
svcudp_create(fd)
	register int fd;
{
	SVCXPRT *dummy;

	trace2(TR_svcudp_create, 0, fd);
	dummy = svc_com_create(fd, UDPMSGSIZE, UDPMSGSIZE, "udp");
	trace2(TR_svcudp_create, 1, fd);
	return (dummy);
}

SVCXPRT *
svcraw_create()
{
	SVCXPRT *dummy;

	trace1(TR_svcraw_create, 0);
	dummy = svc_raw_create();
	trace1(TR_svcraw_create, 1);
	return (dummy);
}

/*
 * Bind a fd to a privileged IP port.
 * This is slightly different from the code in netdir_options
 * because it has a different interface - main thing is that it
 * needs to know its own address.  We also wanted to set the qlen.
 * t_getname() can be used for those purposes and perhaps job can be done.
 */
static int
bindresvport(fd, sin, portp, qlen)
	int fd;
	struct sockaddr_in *sin;
	int *portp;
	int qlen;
{
	int res;
	static short port;
	struct sockaddr_in myaddr;
	int i;
	struct t_bind tbindstr, *tres;
	struct t_info tinfo;

#define	STARTPORT 600
#define	ENDPORT (IPPORT_RESERVED - 1)
#define	NPORTS	(ENDPORT - STARTPORT + 1)

	trace3(TR_bindresvport, 0, fd, qlen);
	/* Removed check of geteuid() here: kernel enforces access privs. */
	if ((i = t_getstate(fd)) != T_UNBND) {
		if (t_errno == TBADF) {
			errno = EBADF;
		}
		if (i != -1) {
			errno = EISCONN;
		}
		trace2(TR_bindresvport, 1, fd);
		return (-1);
	}
	if (sin == (struct sockaddr_in *)NULL) {
		sin = &myaddr;
		get_myaddress(sin);
	} else if (sin->sin_family != AF_INET) {
		errno = EPFNOSUPPORT;
		trace2(TR_bindresvport, 1, fd);
		return (-1);
	}
	/* Get lock to protect pointer to next free port number. */
	MUTEX_LOCK(&__bindresvport_lock);
	if (port == 0)
		port = (getpid() % NPORTS) + STARTPORT;
	res = -1;
	errno = EADDRINUSE;
	/* Transform sockaddr_in to netbuf */
	if (t_getinfo(fd, &tinfo) == -1) {
		trace2(TR_bindresvport, 1, fd);
		return (-1);
	}
	tres = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR);
	if (tres == NULL) {
		trace2(TR_bindresvport, 1, fd);
		return (-1);
	}

	tbindstr.qlen = qlen;
	tbindstr.addr.buf = (char *)sin;
	tbindstr.addr.len = tbindstr.addr.maxlen = _rpc_get_a_size(tinfo.addr);
	sin = (struct sockaddr_in *)tbindstr.addr.buf;

	for (i = 0; i < NPORTS && errno == EADDRINUSE; i++) {
		sin->sin_port = htons(port++);
		if (port > ENDPORT)
			port = STARTPORT;
		res = t_bind(fd, &tbindstr, tres);
		if (res == 0) {
			if ((tbindstr.addr.len == tres->addr.len) &&
				(memcmp(tbindstr.addr.buf, tres->addr.buf,
					(int)tres->addr.len) == 0))
				break;
			(void) t_unbind(fd);
		}
	}
	/* Release lock that protects pointer to next free port number. */
	MUTEX_UNLOCK(&__bindresvport_lock);

	if (res == 0)
		*portp = sin->sin_port;
	(void) t_free((char *)tres, T_BIND);
	trace2(TR_bindresvport, 1, fd);
	return (res);
}

/*
 * Get clients IP address.
 * don't use gethostbyname, which would invoke yellow pages
 * Remains only for backward compatibility reasons.
 * Used mainly by the portmapper so that it can register
 * with itself. Also used by pmap*() routines
 */
int
get_myaddress(addr)
	struct sockaddr_in *addr;
{
	trace1(TR_get_myaddress, 0);
	memset((char *)addr, 0, sizeof (struct sockaddr_in));
	addr->sin_port = htons(PMAPPORT);
	addr->sin_family = AF_INET;
	trace1(TR_get_myaddress, 1);
	return (0);
}

/*
 * For connectionless "udp" transport. Obsoleted by rpc_call().
 */
callrpc(host, prognum, versnum, procnum, inproc, in, outproc, out)
	char *host;
	u_long prognum, versnum, procnum;
	xdrproc_t inproc, outproc;
	char *in, *out;
{
	int dummy;

	trace4(TR_callrpc, 0, prognum, versnum, procnum);
	dummy = (int)rpc_call(host, prognum, versnum, procnum, inproc,
				in, outproc, out, "udp");
	trace4(TR_callrpc, 1, prognum, versnum, procnum);
	return (dummy);
}

/*
 * For connectionless kind of transport. Obsoleted by rpc_reg()
 */
registerrpc(prognum, versnum, procnum, progname, inproc, outproc)
	u_long prognum, versnum, procnum;
	char *(*progname)();
	xdrproc_t inproc, outproc;
{
	int dummy;

	trace4(TR_registerrpc, 0, prognum, versnum, procnum);
	dummy = rpc_reg(prognum, versnum, procnum, progname, inproc,
				outproc, "udp");
	trace4(TR_registerrpc, 1, prognum, versnum, procnum);
	return (dummy);
}

/*
 * All the following clnt_broadcast stuff is convulated; it supports
 * the earlier calling style of the callback function
 */
static resultproc_t clnt_broadcast_result;

/*
 * Need to translate the netbuf address into sockaddr_in address.
 * Dont care about netid here.
 */
static bool_t
rpc_wrap_bcast(resultp, addr, nconf)
	char *resultp;		/* results of the call */
	struct netbuf *addr;	/* address of the guy who responded */
	struct netconfig *nconf; /* Netconf of the transport */
{
	static bool_t dummy;

	trace1(TR_rpc_wrap_bcast, 0);
	dummy = (*clnt_broadcast_result)(resultp,
				(struct sockaddr_in *)addr->buf);
	trace1(TR_rpc_wrap_bcast, 1);
	return (dummy);
}

/*
 * Broadcasts on UDP transport. Obsoleted by rpc_broadcast().
 */
enum clnt_stat
clnt_broadcast(prog, vers, proc, xargs, argsp, xresults, resultsp, eachresult)
	u_long		prog;		/* program number */
	u_long		vers;		/* version number */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	resultproc_t	eachresult;	/* call with each result obtained */
{
	enum clnt_stat dummy;

	trace4(TR_clnt_broadcast, 0, prog, vers, proc);
	clnt_broadcast_result = eachresult;
	dummy = rpc_broadcast(prog, vers, proc, xargs, argsp, xresults,
				resultsp, (resultproc_t) rpc_wrap_bcast, "udp");
	trace4(TR_clnt_broadcast, 1, prog, vers, proc);
	return (dummy);
}

/*
 * Create the client des authentication object. Obsoleted by
 * authdes_seccreate().
 */
AUTH *
authdes_create(servername, window, syncaddr, ckey)
	char *servername;		/* network name of server */
	u_int window;			/* time to live */
	struct sockaddr_in *syncaddr;	/* optional hostaddr to sync with */
	des_block *ckey;		/* optional conversation key to use */
{
	char *hostname = NULL;
	AUTH *dummy;

	trace2(TR_authdes_create, 0, window);
	if (syncaddr) {
		/*
		 * Change addr to hostname, because that is the way
		 * new interface takes it.
		 */
		struct netconfig *nconf;
		struct netbuf nb_syncaddr;
		struct nd_hostservlist *hlist;
		AUTH *nauth;
		int fd;
		struct t_info tinfo;

		if ((nconf = _rpc_getconfip("udp")) == NULL &&
		    (nconf = _rpc_getconfip("tcp")) == NULL)
			goto fallback;

		/* Transform sockaddr_in to netbuf */
		if ((fd = t_open(nconf->nc_device, O_RDWR, &tinfo)) == -1) {
			(void) freenetconfigent(nconf);
			goto fallback;
		}
		(void) t_close(fd);
		nb_syncaddr.maxlen = nb_syncaddr.len =
			_rpc_get_a_size(tinfo.addr);
		nb_syncaddr.buf = (char *)syncaddr;
		if (netdir_getbyaddr(nconf, &hlist, &nb_syncaddr)) {
			(void) freenetconfigent(nconf);
			goto fallback;
		}
		if (hlist && hlist->h_cnt > 0 && hlist->h_hostservs)
			hostname = hlist->h_hostservs->h_host;
		nauth = authdes_seccreate(servername, window, hostname, ckey);
		(void) netdir_free((char *)hlist, ND_HOSTSERVLIST);
		(void) freenetconfigent(nconf);
		trace2(TR_authdes_create, 1, window);
		return (nauth);
	}
fallback:
	dummy = authdes_seccreate(servername, window, hostname, ckey);
	trace2(TR_authdes_create, 1, window);
	return (dummy);
}

#endif /* PORTMAP */

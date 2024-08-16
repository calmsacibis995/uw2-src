/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/pmap_clnt.c	1.4.9.4"
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
 * pmap_clnt.c
 * interface to pmap rpc service.
 *
 */
#ifdef PORTMAP

#include <string.h>
#include <rpc/rpc.h>
#include <rpc/nettype.h>
#include "trace.h"
#include <netdir.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#include <rpc/pmap_rmt.h>
#include <sys/syslog.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "rpc_mt.h"

#undef rpc_createerr	/* Need to pass automatic to set_rpc_createerr() */

static struct timeval timeout = { 5, 0 };
static struct timeval tottimeout = { 60, 0 };
static struct timeval rmttimeout = { 3, 0 };

/*
 * Set a mapping between program, version and port.
 * Calls the pmap service remotely to do the mapping.
 */
bool_t
pmap_set(u_long program, u_long version, int protocol, u_short port)
{
	bool_t rslt;
	struct netbuf *na;
	struct netconfig *nconf;
	char buf[32];

	trace1(TR_pmap_set, 0);
	if ((protocol != IPPROTO_UDP) && (protocol != IPPROTO_TCP)) {
		trace1(TR_pmap_set, 1);
		return (FALSE);
	}
	nconf = _rpc_getconfip(protocol == IPPROTO_UDP ? "udp" : "tcp");
	if (! nconf) {
		trace1(TR_pmap_set, 1);
		return (FALSE);
	}
	sprintf(buf, "0.0.0.0.%d.%d", port >> 8 & 0xff, port & 0xff);
	na = uaddr2taddr(nconf, buf);
	if (! na) {
		freenetconfigent(nconf);
		trace1(TR_pmap_set, 1);
		return (FALSE);
	}
	rslt = rpcb_set(program, version, nconf, na);
	netdir_free((char *)na, ND_ADDR);
	freenetconfigent(nconf);
	trace1(TR_pmap_set, 1);
	return (rslt);
}

/*
 * Remove the mapping between program, version and port.
 * Calls the pmap service remotely to do the un-mapping.
 */
bool_t
pmap_unset(program, version)
	u_long program;
	u_long version;
{
	struct netconfig *nconf;
	bool_t udp_rslt = FALSE;
	bool_t tcp_rslt = FALSE;

	trace1(TR_pmap_unset, 0);
	nconf = _rpc_getconfip("udp");
	if (nconf) {
		udp_rslt = rpcb_unset(program, version, nconf);
		freenetconfigent(nconf);
	}
	nconf = _rpc_getconfip("tcp");
	if (nconf) {
		tcp_rslt = rpcb_unset(program, version, nconf);
		freenetconfigent(nconf);
	}
	/*
	 * XXX: The call may still succeed even if only one of the
	 * calls succeeded.  This was the best that could be
	 * done for backward compatibility.
	 */
	trace1(TR_pmap_unset, 1);
	return (tcp_rslt || udp_rslt);
}

/*
 * Find the mapped port for program, version.
 * Calls the pmap service remotely to do the lookup.
 * Returns 0 if no map exists.
 *
 * XXX: It talks only to the portmapper and not to the rpcbind
 * service.  There may be implementations out there which do not
 * run portmapper as a part of rpcbind.
 */
u_short
pmap_getport(address, program, version, protocol)
	struct sockaddr_in *address;
	u_long program;
	u_long version;
	u_int protocol;
{
	u_short port = 0;
	int fd = RPC_ANYFD;
	register CLIENT *client;
	struct pmap parms;
	rpc_createerr_t rpc_createerr = { 0 };

	trace1(TR_pmap_getport, 0);
	address->sin_port = htons(PMAPPORT);
	client = clntudp_bufcreate(address, PMAPPROG, PMAPVERS, timeout,
				&fd, RPCSMALLMSGSIZE, RPCSMALLMSGSIZE);
	if (client != (CLIENT *)NULL) {
		parms.pm_prog = program;
		parms.pm_vers = version;
		parms.pm_prot = protocol;
		parms.pm_port = 0;	/* not needed or used */
		if (CLNT_CALL(client, PMAPPROC_GETPORT, (xdrproc_t) xdr_pmap,
			(caddr_t) &parms, (xdrproc_t) xdr_u_short,
			    (caddr_t) &port, tottimeout) != RPC_SUCCESS) {
			rpc_createerr.cf_stat = RPC_PMAPFAILURE;
			clnt_geterr(client, &rpc_createerr.cf_error);
			set_rpc_createerr(rpc_createerr);
		} else if (port == 0) {
			rpc_createerr.cf_stat = RPC_PROGNOTREGISTERED;
			set_rpc_createerr(rpc_createerr);
		}
		CLNT_DESTROY(client);
	}
	address->sin_port = 0;
	trace1(TR_pmap_getport, 1);
	return (port);
}

/*
 * Get a copy of the current port maps.
 * Calls the pmap service remotely to do get the maps.
 */
struct pmaplist *
pmap_getmaps(address)
	struct sockaddr_in *address;
{
	pmaplist_ptr head = (pmaplist_ptr)NULL;
	int fd = RPC_ANYFD;
	struct timeval minutetimeout;
	register CLIENT *client;

	trace1(TR_pmap_getmaps, 0);
	minutetimeout.tv_sec = 60;
	minutetimeout.tv_usec = 0;
	address->sin_port = htons(PMAPPORT);
	client = clnttcp_create(address, PMAPPROG, PMAPVERS, &fd, 50, 500);
	if (client != (CLIENT *)NULL) {
		if (CLNT_CALL(client, PMAPPROC_DUMP, (xdrproc_t) xdr_void,
			    (caddr_t) NULL, (xdrproc_t) xdr_pmaplist_ptr,
			    (caddr_t) &head, minutetimeout) != RPC_SUCCESS) {
			char *s;

			s = clnt_sperror(client, "RPC: pmap_getmaps");
			if (s != NULL)
				(void) syslog(LOG_ERR, s);
		}
		CLNT_DESTROY(client);
	}
	address->sin_port = 0;
	trace1(TR_pmap_getmaps, 1);
	return ((struct pmaplist *)head);
}

/*
 * pmapper remote-call-service interface.
 * This routine is used to call the pmapper remote call service
 * which will look up a service program in the port maps, and then
 * remotely call that routine with the given parameters. This allows
 * programs to do a lookup and call in one step.
 */
enum clnt_stat
pmap_rmtcall(addr, prog, vers, proc, xdrargs, argsp, xdrres, resp,
		tout, port_ptr)
	struct sockaddr_in *addr;
	u_long prog, vers, proc;
	xdrproc_t xdrargs, xdrres;
	caddr_t argsp, resp;
	struct timeval tout;
	u_long *port_ptr;
{
	int fd = RPC_ANYFD;
	register CLIENT *client;
	struct p_rmtcallargs a;
	struct p_rmtcallres r;
	enum clnt_stat stat;
	short tmp = addr->sin_port;

	trace1(TR_pmap_rmtcall, 0);
	addr->sin_port = htons(PMAPPORT);
	client = clntudp_create(addr, PMAPPROG, PMAPVERS, rmttimeout, &fd);
	if (client != (CLIENT *)NULL) {
		a.prog = prog;
		a.vers = vers;
		a.proc = proc;
		a.args.args_val = argsp;
		a.xdr_args = xdrargs;
		r.res.res_val = resp;
		r.xdr_res = xdrres;
		stat = CLNT_CALL(client, PMAPPROC_CALLIT,
				(xdrproc_t)xdr_rmtcallargs,
				(caddr_t) &a, (xdrproc_t) xdr_rmtcallres,
				(caddr_t) &r, tout);
		CLNT_DESTROY(client);
	} else {
		stat = RPC_FAILED;
	}
	addr->sin_port = tmp;
	*port_ptr = r.port;
	trace1(TR_pmap_rmtcall, 1);
	return (stat);
}
#endif /* PORTMAP */

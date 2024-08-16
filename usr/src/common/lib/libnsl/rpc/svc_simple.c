/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/svc_simple.c	1.4.9.3"
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
 * svc_simple.c
 * Simplified front end to rpc.
 *
 * This interface creates a virtual listener for all the services
 * started thru rpc_reg(). It listens on the same endpoint for
 * all the services and then executes the corresponding service
 * for the given prognum and procnum.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include "trace.h"
#include <sys/syslog.h>
#include <rpc/nettype.h>
#include "rpc_mt.h"
#include <xti.h>

static struct proglst {
	char *(*p_progname)();
	u_long p_prognum;
	u_long p_versnum;
	u_long p_procnum;
	SVCXPRT *p_transp;
	char *p_netid;
	char *p_xdrbuf;
	int p_recvsz;
	xdrproc_t p_inproc, p_outproc;
	struct proglst *p_nxt;
} *proglst;

static void universal();

/*
 * For simplified, easy to use kind of rpc interfaces.
 * nettype indicates the type of transport on which the service will be
 * listening. Used for conservation of the system resource. Only one
 * handle is created for all the services (actually one of each netid)
 * and same xdrbuf is used for same netid. The size of the arguments
 * is also limited by the recvsize for that transport, even if it is
 * a COTS transport. This may be wrong, but for cases like these, they
 * should not use the simplified interfaces like this.
 */

rpc_reg(prognum, versnum, procnum, progname, inproc, outproc, nettype)
	u_long prognum;			/* program number */
	u_long versnum;			/* version number */
	u_long procnum;			/* procedure number */
	char *(*progname)();		/* Server routine */
	xdrproc_t inproc, outproc;	/* in/out XDR procedures */
	char *nettype;			/* nettype */
{
	struct netconfig *nconf;
	int done = FALSE;
	void *handle;

	trace4(TR_rpc_reg, 0, prognum, versnum, procnum);
	if (procnum == NULLPROC) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:115",
			"rpc_reg: can't reassign procedure number %d"),
		    NULLPROC);
		trace4(TR_rpc_reg, 1, prognum, versnum, procnum);
		return (-1);
	}

	if (nettype == NULL)
		nettype = "netpath";		/* The default behavior */
	if ((handle = _rpc_setconf(nettype)) == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:116",
			"rpc_reg: can't find appropriate transport"));
		return (-1);
	}
	MUTEX_LOCK(&__list_lock);
	while (nconf = _rpc_getconf(handle)) {
		struct proglst *pl;
		SVCXPRT *svcxprt;
		int madenow;
		u_int recvsz;
		char *xdrbuf;
		char *netid;

		madenow = FALSE;
		svcxprt = (SVCXPRT *)NULL;
		for (pl = proglst; pl; pl = pl->p_nxt)
			if (strcmp(pl->p_netid, nconf->nc_netid) == 0) {
				svcxprt = pl->p_transp;
				xdrbuf = pl->p_xdrbuf;
				recvsz = pl->p_recvsz;
				netid = pl->p_netid;
				break;
			}

		if (svcxprt == (SVCXPRT *)NULL) {
			struct t_info tinfo;

			svcxprt = svc_tli_create(RPC_ANYFD, nconf,
					(struct t_bind *)NULL, 0, 0);
			if (svcxprt == (SVCXPRT *)NULL)
				continue;
			if (t_getinfo(svcxprt->xp_fd, &tinfo) == -1) {
				(void) syslog(LOG_ERR,
				    gettxt("uxnsl:117",
					"rpc_reg: t_getinfo() failed: %s"),
				    t_strerror(t_errno));
				SVC_DESTROY(svcxprt);
				continue;
			}
			if ((recvsz = _rpc_get_t_size(0, tinfo.tsdu)) == 0) {
				(void) syslog(LOG_ERR,
				    gettxt("uxnsl:118",
					"rpc_reg: unsupported transport size"));
				SVC_DESTROY(svcxprt);
				continue;
			}
			if (((xdrbuf = malloc((unsigned)recvsz)) == NULL) ||
				((netid = strdup(nconf->nc_netid)) == NULL)) {
				(void) syslog(LOG_ERR, 
				    gettxt("uxnsl:32", "%s: out of memory"),
				    "rpc_reg");
				SVC_DESTROY(svcxprt);
				break;
			}
			madenow = TRUE;
		}
		/*
		 * Check if this (program, version, netid) had already been
		 * registered.  The check may save a few RPC calls to rpcbind
		 */
		for (pl = proglst; pl; pl = pl->p_nxt)
			if ((pl->p_prognum == prognum) &&
				(pl->p_versnum == versnum) &&
				(strcmp(pl->p_netid, netid) == 0))
				break;
		if (pl == NULL) { /* Not yet */
			(void) rpcb_unset(prognum, versnum, nconf);
		} else {
			/* so that svc_reg does not call rpcb_set() */
			nconf = NULL;
		}

		if (!svc_reg(svcxprt, prognum, versnum, universal, nconf)) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:119",
		     "rpc_reg: couldn't register program %d version %d for %s"),
			    prognum, versnum, netid);
			if (madenow) {
				SVC_DESTROY(svcxprt);
				free(xdrbuf);
				free(netid);
			}
			continue;
		}

		pl = (struct proglst *)malloc(sizeof (struct proglst));
		if (pl == (struct proglst *)NULL) {
			(void) syslog(LOG_ERR, 
			    gettxt("uxnsl:32", "%s: out of memory"),
			    "rpc_reg");
			if (madenow) {
				SVC_DESTROY(svcxprt);
				free(xdrbuf);
				free(netid);
			}
			break;
		}
		pl->p_progname = progname;
		pl->p_prognum = prognum;
		pl->p_versnum = versnum;
		pl->p_procnum = procnum;
		pl->p_inproc = inproc;
		pl->p_outproc = outproc;
		pl->p_transp = svcxprt;
		pl->p_xdrbuf = xdrbuf;
		pl->p_recvsz = recvsz;
		pl->p_netid = netid;
		pl->p_nxt = proglst;
		proglst = pl;
		done = TRUE;
	}
	MUTEX_UNLOCK(&__list_lock);
	_rpc_endconf(handle);

	if (done == FALSE) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:120",
			"rpc_reg: can't find suitable transport for %s"),
		    nettype);
		trace4(TR_rpc_reg, 1, prognum, versnum, procnum);

		return (-1);
	}
	trace4(TR_rpc_reg, 1, prognum, versnum, procnum);
	return (0);
}

/*
 * The universal handler for the services registered using registerrpc.
 * It handles both the connectionless and the connection oriented cases.
 */

static void
universal(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	u_long prog, vers, proc;
	char *outdata;
	char *xdrbuf;
	struct proglst *pl;

	/*
	 * enforce "procnum 0 is echo" convention
	 */
	trace1(TR_universal, 0);
	if (rqstp->rq_proc == NULLPROC) {
		if (svc_sendreply(transp, (xdrproc_t) xdr_void,
			(char *)NULL) == FALSE) {

			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:121", "svc_sendreply failed"));
		}
		trace1(TR_universal, 1);
		return;
	}
	prog = rqstp->rq_prog;
	vers = rqstp->rq_vers;
	proc = rqstp->rq_proc;
	for (pl = proglst; pl; pl = pl->p_nxt)
		if (pl->p_prognum == prog && pl->p_procnum == proc &&
			pl->p_versnum == vers &&
			(strcmp(pl->p_netid, transp->xp_netid) == 0)) {
			/* decode arguments into a CLEAN buffer */
			xdrbuf = pl->p_xdrbuf;
			/* Zero the arguments: reqd ! */
			(void) memset(xdrbuf, 0, sizeof (pl->p_recvsz));
			/*
			 * Assuming that sizeof (xdrbuf) would be enough
			 * for the arguments; if not then the program
			 * may bomb. BEWARE!
			 */
			if (!svc_getargs(transp, pl->p_inproc, xdrbuf)) {
				svcerr_decode(transp);
				trace1(TR_universal, 1);
				return;
			}
			outdata = (*(pl->p_progname))(xdrbuf);
			if (outdata == NULL &&
				pl->p_outproc != (xdrproc_t) xdr_void){
				/* there was an error */
				trace1(TR_universal, 1);
				return;
			}
			if (!svc_sendreply(transp, pl->p_outproc, outdata)) {
				(void) syslog(LOG_ERR, (const char *)
				    gettxt("uxnsl:122",
		     "rpc: rpc_reg: trouble replying to program %d version %d"),
				    prog, vers);
				trace1(TR_universal, 1);
				return;
			}
			/* free the decoded arguments */
			(void) svc_freeargs(transp, pl->p_inproc, xdrbuf);
			trace1(TR_universal, 1);
			return;
		};
	/* This should never happen */
	(void) syslog(LOG_ERR, (const char *)
	    gettxt("uxnsl:123",
		"rpc: rpc_reg: never registered program %d version %d"),
	    prog, vers);
	trace1(TR_universal, 1);
	return;
}




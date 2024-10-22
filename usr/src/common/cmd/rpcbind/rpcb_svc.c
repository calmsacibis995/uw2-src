/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rpcbind:rpcb_svc.c	1.13.11.6"
#ident  "$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

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
 */
/*
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

/*
 * rpcb_svc.c
 * The server procedure for the version 3 rpcbind (TLI).
 *
 * It maintains a separate list of all the registered services with the
 * version 3 of rpcbind.
 */
#include <stdio.h>
#include <sys/types.h>
#include <rpc/rpc.h>
#include <rpc/rpcb_prot.h>
#include <netconfig.h>
#include <sys/syslog.h>
#include <netdir.h>
#include <stdlib.h>
#include <pfmt.h>
#include <unistd.h>
#include "rpcbind.h"

/*
 * Called by svc_getreqset. There is a separate server handle for
 * every transport that it waits on.
 */
void
rpcb_service_3(rqstp, transp)
	register struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	union {
		RPCB rpcbproc_set_3_arg;
		RPCB rpcbproc_unset_3_arg;
		RPCB rpcbproc_getaddr_3_arg;
		struct rpcb_rmtcallargs rpcbproc_callit_3_arg;
		char *rpcbproc_uaddr2taddr_3_arg;
		struct netbuf rpcbproc_taddr2uaddr_3_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();

	rpcbs_procinfo((u_long) RPCBVERS_3_STAT, rqstp->rq_proc);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		/*
		 * Null proc call
		 */
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_NULL\n");
#endif
		(void) svc_sendreply(transp, (xdrproc_t)xdr_void, (char *)NULL);
		return;

	case RPCBPROC_SET:
		/*
		 * Check to see whether the message came from
		 * loopback transports (for security reasons)
		 */
		if (strcasecmp(transp->xp_netid, loopback_dg) &&
			strcasecmp(transp->xp_netid, loopback_vc) &&
			strcasecmp(transp->xp_netid, loopback_vc_ord)) {
			char *uaddr;

			uaddr = taddr2uaddr(rpcbind_get_conf(transp->xp_netid),
					svc_getrpccaller(transp));
			(void)strcpy(syslogmsgp,
				     gettxt(":58",
					   "Non-local attempt to set from %s"));
			syslog(LOG_ERR, syslogmsg, uaddr);
			free(uaddr);
			svcerr_weakauth(transp);
			return;
		}
		xdr_argument = xdr_rpcb;
		xdr_result = xdr_bool;
		local = (char *(*)()) rpcbproc_set_com;
		break;

	case RPCBPROC_UNSET:
		/*
		 * Check to see whether the message came from
		 * loopback transports (for security reasons)
		 */
		if (strcasecmp(transp->xp_netid, loopback_dg) &&
			strcasecmp(transp->xp_netid, loopback_vc) &&
			strcasecmp(transp->xp_netid, loopback_vc_ord)) {
			char *uaddr;

			uaddr = taddr2uaddr(rpcbind_get_conf(transp->xp_netid),
					svc_getrpccaller(transp));
			(void)strcpy(syslogmsgp,
				     gettxt(":60", 
					 "Non-local attempt to unset from %s"));
			syslog(LOG_ERR, syslogmsg, uaddr);
			free(uaddr);
			svcerr_weakauth(transp);
			return;
		}
		xdr_argument = xdr_rpcb;
		xdr_result = xdr_bool;
		local = (char *(*)()) rpcbproc_unset_com;
		break;

	case RPCBPROC_GETADDR:
		xdr_argument = xdr_rpcb;
		xdr_result = xdr_wrapstring;
		local = (char *(*)()) rpcbproc_getaddr_3;
		break;

	case RPCBPROC_DUMP:
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_DUMP\n");
#endif
		xdr_argument = xdr_void;
		xdr_result = xdr_rpcblist_ptr;
		local = (char *(*)()) rpcbproc_dump_3;
		break;

	case RPCBPROC_CALLIT:
		rpcbproc_callit_com(rqstp, transp, rqstp->rq_proc, RPCBVERS);
		return;

	case RPCBPROC_GETTIME:
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_GETTIME\n");
#endif
		xdr_argument = xdr_void;
		xdr_result = xdr_u_long;
		local = (char *(*)()) rpcbproc_gettime_com;
		break;

	case RPCBPROC_UADDR2TADDR:
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_UADDR2TADDR\n");
#endif
		xdr_argument = xdr_wrapstring;
		xdr_result = xdr_netbuf;
		local = (char *(*)()) rpcbproc_uaddr2taddr_com;
		break;

	case RPCBPROC_TADDR2UADDR:
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_TADDR2UADDR\n");
#endif
		xdr_argument = xdr_netbuf;
		xdr_result = xdr_wrapstring;
		local = (char *(*)()) rpcbproc_taddr2uaddr_com;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	(void) memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, (xdrproc_t) xdr_argument,
				(char *) &argument)) {
		svcerr_decode(transp);
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			     ":19:Could not decode for %d\n", rqstp->rq_proc);
		}
		return;
	}
	result = (*local)(&argument, rqstp, transp, RPCBVERS);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t)xdr_result,
						result)) {
		svcerr_systemerr(transp);
		if (debugging) {
			pfmt(stderr, MM_ERROR, ":38:Could not send reply\n");
			rpcbind_abort();
		}
	}
	if (!svc_freeargs(transp, (xdrproc_t)xdr_argument, (char *)
				&argument)) {
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			     ":65:Unable to free arguments\n");
			rpcbind_abort();
		}
	}
}

/*
 * Lookup the mapping for a program, version and return its
 * address. Assuming that the caller wants the address of the
 * server running on the transport on which the request came.
 *
 * We also try to resolve the universal address in terms of
 * address of the caller.
 */
/* ARGSUSED */
static char **
rpcbproc_getaddr_3(regp, rqstp, transp)
	RPCB *regp;
	struct svc_req *rqstp;	/* Not used here */
	SVCXPRT *transp;
{
#ifdef RPCBIND_DEBUG
	char *uaddr;

	uaddr = taddr2uaddr(rpcbind_get_conf(transp->xp_netid),
			    svc_getrpccaller(transp));
	fprintf(stderr, "RPCB_GETADDR request for (%lu, %lu, %s) from %s : ",
		regp->r_prog, regp->r_vers, regp->r_netid, uaddr);
	free(uaddr);
#endif
	return (rpcbproc_getaddr_com(regp, rqstp, transp, RPCBVERS,
					(u_long) RPCB_ALLVERS));
}

/* ARGSUSED */
static rpcblist_ptr *
rpcbproc_dump_3()
{
	return ((rpcblist_ptr *)&list_rbl);
}

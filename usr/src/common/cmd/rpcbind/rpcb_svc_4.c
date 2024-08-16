/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rpcbind:rpcb_svc_4.c	1.2"
#ident  "$Header: $"

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
 * rpcb_svc_4.c
 * The server procedure for the version 4 rpcbind.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netconfig.h>
#include <netdir.h>
#include <pfmt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <rpc/rpc.h>
#include <sys/syslog.h>
#include "rpcbind.h"

static void free_rpcb_entry_list();

/*
 * Called by svc_getreqset. There is a separate server handle for
 * every transport that it waits on.
 */
void
rpcb_service_4(rqstp, transp)
	register struct svc_req *rqstp;
	register SVCXPRT *transp;
{
	union {
		rpcb rpcbproc_set_4_arg;
		rpcb rpcbproc_unset_4_arg;
		rpcb rpcbproc_getaddr_4_arg;
		char *rpcbproc_uaddr2taddr_4_arg;
		struct netbuf rpcbproc_taddr2uaddr_4_arg;
	} argument;
	char *result;
	bool_t (*xdr_argument)(), (*xdr_result)();
	char *(*local)();

	rpcbs_procinfo(RPCBVERS_4_STAT, rqstp->rq_proc);

	switch (rqstp->rq_proc) {
	case NULLPROC:
		/*
		 * Null proc call
		 */
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_NULL\n");
#endif
		(void) svc_sendreply(transp, (xdrproc_t) xdr_void,
					(char *)NULL);
		return;

	case RPCBPROC_SET:
		/*
		 * Check to see whether the message came from
		 * loopback transports (for security reasons)
		 */
		if (strcasecmp(transp->xp_netid, loopback_dg) &&
			strcasecmp(transp->xp_netid, loopback_vc) &&
			strcasecmp(transp->xp_netid, loopback_vc_ord)) {
			(void)strcpy(syslogmsgp,
				     gettxt(":57", "Non-local attempt to set"));
			syslog(LOG_ERR, syslogmsg);
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
			(void)strcpy(syslogmsgp,
				   gettxt(":59", "Non-local attempt to unset"));
			syslog(LOG_ERR, syslogmsg);
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
		local = (char *(*)()) rpcbproc_getaddr_4;
		break;

	case RPCBPROC_GETVERSADDR:
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_GETVERSADDR\n");
#endif
		xdr_argument = xdr_rpcb;
		xdr_result = xdr_wrapstring;
		local = (char *(*)()) rpcbproc_getversaddr_4;
		break;

	case RPCBPROC_DUMP:
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_DUMP\n");
#endif
		xdr_argument = xdr_void;
		xdr_result = xdr_rpcblist_ptr;
		local = (char *(*)()) rpcbproc_dump_4;
		break;

	case RPCBPROC_INDIRECT:
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_INDIRECT\n");
#endif
		rpcbproc_callit_com(rqstp, transp, rqstp->rq_proc, RPCBVERS4);
		return;

/*	case RPCBPROC_CALLIT: */
	case RPCBPROC_BCAST:
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_BCAST\n");
#endif
		rpcbproc_callit_com(rqstp, transp, rqstp->rq_proc, RPCBVERS4);
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

	case RPCBPROC_GETADDRLIST:
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_GETADDRLIST\n");
#endif
		xdr_argument = xdr_rpcb;
		xdr_result = xdr_rpcb_entry_list_ptr;
		local = (char *(*)()) rpcbproc_getaddrlist_4;
		break;

	case RPCBPROC_GETSTAT:
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "RPCBPROC_GETSTAT\n");
#endif
		xdr_argument = xdr_void;
		xdr_result = xdr_rpcb_stat_byvers;
		local = (char *(*)()) rpcbproc_getstat;
		break;

	default:
		svcerr_noproc(transp);
		return;
	}
	memset((char *)&argument, 0, sizeof (argument));
	if (!svc_getargs(transp, (xdrproc_t) xdr_argument,
		(char *)&argument)) {
		svcerr_decode(transp);
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			     ":19:Could not decode for %d\n", rqstp->rq_proc);
		}
		return;
	}
	result = (*local)(&argument, rqstp, transp, RPCBVERS4);
	if (result != NULL && !svc_sendreply(transp, (xdrproc_t) xdr_result,
						result)) {
		svcerr_systemerr(transp);
		if (debugging) {
			pfmt(stderr, MM_ERROR, ":38:Could not send reply\n");
			rpcbind_abort();
		}
	}
	if (!svc_freeargs(transp, (xdrproc_t) xdr_argument,
				(char *)&argument)) {
		if (debugging) {
		      pfmt(stderr, MM_ERROR, ":65:Unable to free arguments\n");
			rpcbind_abort();
		}
	}
	return;
}

/*
 * Lookup the mapping for a program, version and return its
 * address. Assuming that the caller wants the address of the
 * server running on the transport on which the request came.
 * Even if a service with a different version number is available,
 * it will return that address.  The client should check with an
 * clnt_call to verify whether the service is the one that is desired.
 * We also try to resolve the universal address in terms of
 * address of the caller.
 */
/* ARGSUSED */
static char **
rpcbproc_getaddr_4(regp, rqstp, transp, rpcbversnum)
	rpcb *regp;
	struct svc_req *rqstp;	/* Not used here */
	SVCXPRT *transp;
	int rpcbversnum; /* unused here */
{
#ifdef RPCBIND_DEBUG
	char *uaddr;

	uaddr =	taddr2uaddr(rpcbind_get_conf(transp->xp_netid),
			    svc_getrpccaller(transp));
	fprintf(stderr, "RPCB_GETADDR request for (%lu, %lu, %s) from %s : ",
		regp->r_prog, regp->r_vers, regp->r_netid, uaddr);
	free(uaddr);
#endif
	return (rpcbproc_getaddr_com(regp, rqstp, transp, RPCBVERS4,
					(u_long) RPCB_ALLVERS));
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
rpcbproc_getversaddr_4(regp, rqstp, transp)
	rpcb *regp;
	struct svc_req *rqstp;	/* Not used here */
	SVCXPRT *transp;
{
#ifdef RPCBIND_DEBUG
	char *uaddr;

	uaddr = taddr2uaddr(rpcbind_get_conf(transp->xp_netid),
			    svc_getrpccaller(transp));
	fprintf(stderr, "RPCB_GETVERSADDR rqst for (%lu, %lu, %s) from %s : ",
		regp->r_prog, regp->r_vers, regp->r_netid, uaddr);
	free(uaddr);
#endif
	return (rpcbproc_getaddr_com(regp, rqstp, transp, RPCBVERS4,
					(u_long) RPCB_ONEVERS));
}

/*
 * Lookup the mapping for a program, version and return the
 * addresses for all transports in the current transport family.
 * We return a merged address.
 */
/* ARGSUSED */
static rpcb_entry_list_ptr *
rpcbproc_getaddrlist_4(regp, rqstp, transp)
	rpcb *regp;
	struct svc_req *rqstp;	/* Not used here */
	SVCXPRT *transp;
{
	static rpcb_entry_list_ptr rlist;
	register rpcblist_ptr rbl;
	rpcb_entry_list_ptr rp, tail;
	u_long prog, vers;
	rpcb_entry *a;
	struct netconfig *nconf;
	struct netconfig *reg_nconf;
	char *saddr, *maddr;

	free_rpcb_entry_list(&rlist);
	prog = regp->r_prog;
	vers = regp->r_vers;
	reg_nconf = rpcbind_get_conf(transp->xp_netid);
	if (reg_nconf == NULL)
		return (NULL);
	if (*(regp->r_addr) != '\0') {
		saddr = regp->r_addr;
	} else {
		saddr = NULL;
	}
#ifdef RPCBIND_DEBUG
	fprintf(stderr, "r_addr: %s r_netid: %s nc_protofmly: %s\n",
		regp->r_addr, regp->r_netid, reg_nconf->nc_protofmly);
#endif
	for (rbl = list_rbl; rbl != NULL; rbl = rbl->rpcb_next) {
	    if ((rbl->rpcb_map.r_prog == prog) &&
		(rbl->rpcb_map.r_vers == vers)) {
		nconf = rpcbind_get_conf(rbl->rpcb_map.r_netid);
		if (nconf == NULL)
			goto fail;
		if (strcmp(nconf->nc_protofmly, reg_nconf->nc_protofmly)
				!= 0) {
			continue;	/* not same proto family */
		}
#ifdef RPCBIND_DEBUG
	fprintf(stderr, "\tmerge with: %s", rbl->rpcb_map.r_addr);
#endif
		if ((maddr = mergeaddr(transp, rbl->rpcb_map.r_netid,
				rbl->rpcb_map.r_addr, saddr)) == NULL) {
#ifdef RPCBIND_DEBUG
	fprintf(stderr, " FAILED\n");
#endif
			continue;
		} else if (!maddr[0]) {
#ifdef RPCBIND_DEBUG
	fprintf(stderr, " SUCCEEDED, but port died -  maddr: nullstring\n");
#endif
			/* The server died. Unset this combination */
			(void) map_unset(regp, "superuser");
			continue;
		}
#ifdef RPCBIND_DEBUG
	fprintf(stderr, " SUCCEEDED maddr: %s\n", maddr);
#endif
		/*
		 * Add it to rlist.
		 */
		rp = (rpcb_entry_list_ptr)
			malloc((u_int)sizeof (rpcb_entry_list));
		if (rp == NULL)
			goto fail;
		a = &rp->rpcb_entry_map;
		a->r_maddr = maddr;
		a->r_nc_netid = nconf->nc_netid;
		a->r_nc_semantics = nconf->nc_semantics;
		a->r_nc_protofmly = nconf->nc_protofmly;
		a->r_nc_proto = nconf->nc_proto;
		rp->rpcb_entry_next = NULL;
		if (rlist == NULL) {
			rlist = rp;
			tail = rp;
		} else {
			tail->rpcb_entry_next = rp;
			tail = rp;
		}
		rp = NULL;
	    }
	}
#ifdef RPCBIND_DEBUG
	for (rp = rlist; rp; rp = rp->rpcb_entry_next) {
		fprintf(stderr, "\t%s %s\n", rp->rpcb_entry_map.r_maddr,
			rp->rpcb_entry_map.r_nc_proto);
	}
#endif
	return (&rlist);

fail:	free_rpcb_entry_list(&rlist);
	return (NULL);
}

/*
 * Free only the allocated structure, rest is all a pointer to some
 * other data somewhere else.
 */
static void
free_rpcb_entry_list(rlistp)
	rpcb_entry_list_ptr *rlistp;
{
	register rpcb_entry_list_ptr rbl, tmp;

	for (rbl = *rlistp; rbl != NULL; ) {
		tmp = rbl;
		rbl = rbl->rpcb_entry_next;
		free((char *)tmp->rpcb_entry_map.r_maddr);
		free((char *)tmp);
	}
	*rlistp = NULL;
}

/* VARARGS */
static rpcblist_ptr *
rpcbproc_dump_4()
{
	return ((rpcblist_ptr *)&list_rbl);
}

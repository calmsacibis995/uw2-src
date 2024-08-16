/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rpcbind:rpcb_svc_com.c	1.3"
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
 * rpcb_svc_com.c
 * The commom server procedure for the rpcbind.
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
#include <rpc/rpcb_prot.h>
#ifdef PORTMAP
#include <netinet/in.h>
#include <rpc/pmap_prot.h>
#endif /* PORTMAP */
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/poll.h>
#include <sys/stropts.h>
#include <sys/syslog.h>
#include <sys/utsname.h>
#include "rpcbind.h"

bool_t map_set(), map_unset();
char **rpcbproc_getaddr_com();

static bool_t	xdr_opaque_parms();
static char	*getowner();
static u_long	forward_register();
static void	handle_reply();
static int	netbufcmp();
static int	free_slot_by_xid();
static int	free_slot_by_fd();
static int	free_slot_by_index();
static int	check_rmtcalls();
static void	netbuffree();
static void	find_versions();
static struct netbuf	*netbufdup();
static rpcblist_ptr find_service();

static char *nullstring = "";
static int rpcb_rmtcalls;

/*
 * Set a mapping of program, version, netid
 */
/* ARGSUSED */
bool_t *
rpcbproc_set_com(regp, rqstp, transp, rpcbversnum)
	RPCB *regp;
	struct svc_req *rqstp;	/* Not used here */
	SVCXPRT *transp;
	int rpcbversnum;
{
	static bool_t ans;
	char owner[64];

#ifdef RPCBIND_DEBUG
	fprintf(stderr, "RPCB_SET request for (%lu, %lu, %s, %s) : ",
		regp->r_prog, regp->r_vers, regp->r_netid, regp->r_addr);
#endif
	ans = map_set(regp, getowner(transp, owner));
#ifdef RPCBIND_DEBUG
	fprintf(stderr, "%s\n", ans == TRUE ? "succeeded" : "failed");
#endif
	/* XXX: should have used some defined constant here */
	rpcbs_set((u_long) (rpcbversnum - 2), ans);
	return (&ans);
}

bool_t
map_set(regp, owner)
	RPCB *regp;
	char *owner;
{
	RPCB reg, *a;
	rpcblist_ptr rbl, fnd;

	reg = *regp;
	/*
	 * check to see if already used
	 * find_service returns a hit even if
	 * the versions don't match, so check for it
	 */
	fnd = find_service(reg.r_prog, reg.r_vers, reg.r_netid);
	if (fnd && (fnd->rpcb_map.r_vers == reg.r_vers)) {
		if (!strcmp(fnd->rpcb_map.r_addr, reg.r_addr))
			/*
			 * if these match then it is already
			 * registered so just say "OK".
			 */
			return (TRUE);
		else
			return (FALSE);
	}
	/*
	 * add to the end of the list
	 */
	rbl = (rpcblist_ptr) malloc((u_int)sizeof (RPCBLIST));
	if (rbl == (rpcblist_ptr)NULL) {
		return (FALSE);
	}
	a = &(rbl->rpcb_map);
	a->r_prog = reg.r_prog;
	a->r_vers = reg.r_vers;
	a->r_netid = strdup(reg.r_netid);
	a->r_addr = strdup(reg.r_addr);
	a->r_owner = strdup(owner);
	if (!a->r_addr || !a->r_netid || !a->r_owner) {
		if (a->r_netid)
			free((void *) a->r_netid);
		if (a->r_addr)
			free((void *) a->r_addr);
		if (a->r_owner)
			free((void *) a->r_owner);
		free((void *)rbl);
		return (FALSE);
	}
	rbl->rpcb_next = (rpcblist_ptr)NULL;
	if (list_rbl == NULL) {
		list_rbl = rbl;
	} else {
		for (fnd = list_rbl; fnd->rpcb_next;
			fnd = fnd->rpcb_next)
			;
		fnd->rpcb_next = rbl;
	}
#ifdef PORTMAP
	(void) add_pmaplist(regp);
#endif
	return (TRUE);
}

/*
 * Unset a mapping of program, version, netid
 */
/* ARGSUSED */
bool_t *
rpcbproc_unset_com(regp, rqstp, transp, rpcbversnum)
	RPCB *regp;
	struct svc_req *rqstp;	/* Not used here */
	SVCXPRT *transp;
	int rpcbversnum;
{
	static bool_t ans;
	char owner[64];

#ifdef RPCBIND_DEBUG
	fprintf(stderr, "RPCB_UNSET request for (%lu, %lu, %s) : ",
		regp->r_prog, regp->r_vers, regp->r_netid);
#endif
	ans = map_unset(regp, getowner(transp, owner));
#ifdef RPCBIND_DEBUG
	fprintf(stderr, "%s\n", ans == TRUE ? "succeeded" : "failed");
#endif
	/* XXX: should have used some defined constant here */
	rpcbs_unset((u_long) (rpcbversnum - 2), ans);
	return (&ans);
}

bool_t
map_unset(regp, owner)
	RPCB *regp;
	char *owner;
{
	int ans = 0;
	rpcblist_ptr rbl, prev, tmp;

	if (owner == NULL)
		return (0);

	for (prev = NULL, rbl = list_rbl; rbl; ) {
		if ((rbl->rpcb_map.r_prog != regp->r_prog) ||
			(rbl->rpcb_map.r_vers != regp->r_vers) ||
			(regp->r_netid[0] && strcasecmp(regp->r_netid,
				rbl->rpcb_map.r_netid))) {
			/* both rbl & prev move forwards */
			prev = rbl;
			rbl = rbl->rpcb_next;
			continue;
		}
		/*
		 * Check whether appropriate uid. Unset only
		 * if superuser or the owner itself.
		 */
		if (strcmp(owner, "superuser") &&
			strcmp(rbl->rpcb_map.r_owner, owner))
			return (0);
		/* found it; rbl moves forward, prev stays */
		ans = 1;
		tmp = rbl;
		rbl = rbl->rpcb_next;
		if (prev == NULL)
			list_rbl = rbl;
		else
			prev->rpcb_next = rbl;
		free((void *) tmp->rpcb_map.r_addr);
		free((void *) tmp->rpcb_map.r_netid);
		free((void *) tmp->rpcb_map.r_owner);
		free((void *) tmp);
	}
#ifdef PORTMAP
	if (ans)
		(void) del_pmaplist(regp);
#endif
	/*
	 * We return 1 either when the entry was not there or it
	 * was able to unset it.  It can come to this point only if
	 * atleast one of the conditions is true.
	 */
	return (1);
}

char **
rpcbproc_getaddr_com(regp, rqstp, transp, rpcbversnum, verstype)
	RPCB *regp;
	struct svc_req *rqstp;	/* Not used here */
	SVCXPRT *transp;
	u_long rpcbversnum;
	u_long verstype;
{
	static char *uaddr;
	char *saddr = NULL;
	rpcblist_ptr fnd;

	if (uaddr && uaddr[0])
		free((void *) uaddr);
	fnd = find_service(regp->r_prog, regp->r_vers, transp->xp_netid);
	if (fnd && ((verstype == RPCB_ALLVERS) ||
		    (regp->r_vers == fnd->rpcb_map.r_vers))) {
		if (*(regp->r_addr) != '\0') {  /* may contain a hint about */
			saddr = regp->r_addr;   /* the interface that we    */
		}				/* should use */
		if (!(uaddr = mergeaddr(transp, transp->xp_netid,
				fnd->rpcb_map.r_addr, saddr))) {
			/* Try whatever we have */
			uaddr = strdup(fnd->rpcb_map.r_addr);
		} else if (!uaddr[0]) {
			/* The server died. Unset this combination */
			uaddr = nullstring;
			(void) map_unset(regp, "superuser");
		}
	} else {
		uaddr = nullstring;
	}
#ifdef RPCBIND_DEBUG
	fprintf(stderr, "getaddr: %s\n", uaddr);
#endif
	/* XXX: should have used some defined constant here */
	rpcbs_getaddr(rpcbversnum - 2, regp->r_prog, regp->r_vers,
		transp->xp_netid, uaddr);
	return (&uaddr);
}

/* VARARGS */
u_long *
rpcbproc_gettime_com()
{
	static time_t curtime;

	(void) time(&curtime);
	return ((u_long *)&curtime);
}

/*
 * Convert uaddr to taddr. Should be used only by
 * local servers/clients. (kernel level stuff only)
 */
/* ARGSUSED */
struct netbuf *
rpcbproc_uaddr2taddr_com(uaddrp, rqstp, transp, rpcbversnum)
	char **uaddrp;
	struct svc_req *rqstp;	/* Not used here */
	SVCXPRT *transp;
	int rpcbversnum;	/* Not used here */
{
	struct netconfig *nconf;
	static struct netbuf nbuf;
	static struct netbuf *taddr;

	if (taddr) {
		free((void *) taddr->buf);
		free((void *) taddr);
	}
	if (((nconf = rpcbind_get_conf(transp->xp_netid)) == NULL) ||
	    ((taddr = uaddr2taddr(nconf, *uaddrp)) == NULL)) {
		(void) memset((char *)&nbuf, 0, sizeof (struct netbuf));
		return (&nbuf);
	}
	return (taddr);
}

/*
 * Convert taddr to uaddr. Should be used only by
 * local servers/clients. (kernel level stuff only)
 */
/* ARGSUSED */
char **
rpcbproc_taddr2uaddr_com(taddr, rqstp, transp, rpcbversnum)
	struct netbuf *taddr;
	struct svc_req *rqstp;	/* Not used here */
	SVCXPRT *transp;
	int rpcbversnum; /* unused */
{
	static char *uaddr;
	struct netconfig *nconf;

	if (uaddr && !uaddr[0])
		free((void *) uaddr);
	if (((nconf = rpcbind_get_conf(transp->xp_netid)) == NULL) ||
		((uaddr = taddr2uaddr(nconf, taddr)) == NULL)) {
		uaddr = nullstring;
	}
	return (&uaddr);
}


/*
 * Stuff for the rmtcall service
 */
struct encap_parms {
	u_long arglen;
	char *args;
};

static bool_t
xdr_encap_parms(xdrs, epp)
	XDR *xdrs;
	struct encap_parms *epp;
{
	return (xdr_bytes(xdrs, &(epp->args), (u_int *) &(epp->arglen), ~0));
}


struct r_rmtcall_args {
	u_long 	rmt_prog;
	u_long 	rmt_vers;
	u_long 	rmt_proc;
	int	rmt_localvers;	/* whether to send port # or uaddr */
	char 	*rmt_uaddr;
	struct encap_parms rmt_args;
};

/*
 * XDR remote call arguments.  It ignores the address part.
 * written for XDR_DECODE direction only
 */
static bool_t
xdr_rmtcall_args(xdrs, cap)
	register XDR *xdrs;
	register struct r_rmtcall_args *cap;
{
	/* does not get the address or the arguments */
	if (xdr_u_long(xdrs, &(cap->rmt_prog)) &&
	    xdr_u_long(xdrs, &(cap->rmt_vers)) &&
	    xdr_u_long(xdrs, &(cap->rmt_proc))) {
		return (xdr_encap_parms(xdrs, &(cap->rmt_args)));
	}
	return (FALSE);
}

/*
 * XDR remote call results along with the address.  Ignore
 * program number, version  number and proc number.
 * Written for XDR_ENCODE direction only.
 */
static bool_t
xdr_rmtcall_result(xdrs, cap)
	register XDR *xdrs;
	register struct r_rmtcall_args *cap;
{
	bool_t result;

#ifdef PORTMAP
	if (cap->rmt_localvers == PMAPVERS) {
		int h1, h2, h3, h4, p1, p2;
		u_long port;

		/* interpret the universal address for TCP/IP */
		if (sscanf(cap->rmt_uaddr, "%d.%d.%d.%d.%d.%d",
			&h1, &h2, &h3, &h4, &p1, &p2) != 6)
			return (FALSE);
		port = ((p1 & 0xff) << 8) + (p2 & 0xff);
		result = xdr_u_long(xdrs, &port);
	} else
#endif
		if ((cap->rmt_localvers == RPCBVERS) ||
		    (cap->rmt_localvers == RPCBVERS4)) {
		result = xdr_wrapstring(xdrs, &(cap->rmt_uaddr));
	} else {
		return (FALSE);
	}
	if (result == TRUE)
		return (xdr_encap_parms(xdrs, &(cap->rmt_args)));
	return (FALSE);
}

/*
 * only worries about the struct encap_parms part of struct r_rmtcall_args.
 * The arglen must already be set!!
 */
static bool_t
xdr_opaque_parms(xdrs, cap)
	XDR *xdrs;
	struct r_rmtcall_args *cap;
{
	return (xdr_opaque(xdrs, cap->rmt_args.args, cap->rmt_args.arglen));
}

/*
 * Call a remote procedure service.  This procedure is very quiet when things
 * go wrong.  The proc is written to support broadcast rpc.  In the broadcast
 * case, a machine should shut-up instead of complain, lest the requestor be
 * overrun with complaints at the expense of not hearing a valid reply.
 * When receiving a request and verifying that the service exists, we
 *
 *	receive the request
 *
 *	open a new TLI endpoint on the same transport on which we received
 *	the original request
 *
 *	remember the original request's XID (which requires knowing the format
 *	of the svc_dg_data structure)
 *
 *	forward the request, with a new XID, to the requested service,
 *	remembering the XID used to send this request (for later use in
 *	reassociating the answer with the original request), the requestor's
 *	address, the file descriptor on which the forwarded request is
 *	made and the service's address.
 *
 *	mark the file descriptor on which we anticipate receiving a reply from
 *	the service and one to select for in our private svc_run procedure
 *
 * At some time in the future, a reply will be received from the service to
 * which we forwarded the request.  At that time, we detect that the socket
 * used was for forwarding (by looking through the finfo structures to see
 * whether the fd corresponds to one of those) and call handle_reply() to
 *
 *	receive the reply
 *
 *	bundle the reply, along with the service's universal address
 *
 *	create a SVCXPRT structure and use a version of svc_sendreply
 *	that allows us to specify the reply XID and destination, send the reply
 *	to the original requestor.
 */

/*	begin kludge XXX */
/*
 * This is from .../libnsl/rpc/svc_dg.c, and is the structure that xprt->xp_p2
 * points to (and shouldn't be here - we should know nothing of its structure).
 */
#define	MAX_OPT_WORDS	32
#define	RPC_BUF_MAX	32768	/* can be raised if required */

struct svc_dg_data {
	struct	netbuf optbuf;
	long	opts[MAX_OPT_WORDS];		/* options */
	u_int	su_iosz;			/* size of send.recv buffer */
	u_long	su_xid;				/* transaction id */
	XDR	su_xdrs;			/* XDR handle */
	char	su_verfbody[MAX_AUTH_BYTES];	/* verifier body */
	char	*su_cache;			/* cached data, NULL if none */
};
#define	getbogus_data(xprt) ((struct svc_dg_data *) (xprt->xp_p2))
/*	end kludge XXX	*/

void
rpcbproc_callit_com(rqstp, transp, reply_type, versnum)
	struct svc_req *rqstp;
	SVCXPRT *transp;
	u_long reply_type;	/* which proc number */
	u_long versnum;	/* which vers was called */
{
	register rpcblist_ptr rbl;
	struct netconfig *nconf;
	struct r_rmtcall_args a;
	char *buf_alloc = NULL;
	char *outbuf_alloc = NULL;
	char buf[RPC_BUF_MAX], outbuf[RPC_BUF_MAX];
	struct netbuf *na = (struct netbuf *) NULL;
	struct t_info tinfo;
	struct t_unitdata tu_data;
	struct rpc_msg call_msg;
	struct svc_dg_data *bd;
	int outlen;
	u_int sendsz;
	XDR outxdr;
	AUTH *auth;
	int fd = -1;
	char *uaddr;
	struct nd_mergearg ma;
	struct utsname utsname;
	struct nd_addrlist *nas;
	struct nd_hostserv hs;
	int stat;

	if (t_getinfo(transp->xp_fd, &tinfo) == -1) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		return;
	}
	if (tinfo.servtype != T_CLTS)
		return;	/* Only datagram type accepted */
	sendsz = _rpc_get_t_size(0, tinfo.tsdu);
	if (sendsz == 0) {	/* data transfer not supported */
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		return;
	}
	/*
	 * Should be multiple of 4 for XDR.
	 */
	sendsz = ((sendsz + 3) / 4) * 4;
	if (sendsz > RPC_BUF_MAX) {
#ifdef	notyet
		buf_alloc = alloca(sendsz);		/* not in IDR2? */
#else
		buf_alloc = malloc(sendsz);
#endif	/* notyet */
		if (buf_alloc == NULL) {
			if (debugging) {
				pfmt(stderr, MM_ERROR, ":56:No memory!");
				fprintf(stderr, "\n");
			}
			if (reply_type == RPCBPROC_INDIRECT)
				svcerr_systemerr(transp);
			return;
		}
		a.rmt_args.args = buf_alloc;
	} else {
		a.rmt_args.args = buf;
	}

	call_msg.rm_xid = 0;	/* For error checking purposes */
	if (!svc_getargs(transp, (xdrproc_t) xdr_rmtcall_args, (char *) &a)) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_decode(transp);
		if (debugging) {
			pfmt(stderr, MM_ERROR, ":23:Could not get arguments\n");
		}
		goto error;
	}

#ifdef RPCBIND_DEBUG
	uaddr = taddr2uaddr(rpcbind_get_conf(transp->xp_netid),
			    svc_getrpccaller(transp));
	fprintf(stderr, "%s %s request for (%lu, %lu, %lu, %s) from %s : ",
		versnum == PMAPVERS ? "pmap_rmtcall" :
		versnum == RPCBVERS ? "rpcb_rmtcall" :
		versnum == RPCBVERS4 ? "rpcb_indirect" : "unknown",
		reply_type == RPCBPROC_INDIRECT ? "indirect" : "callit",
		a.rmt_prog, a.rmt_vers, a.rmt_proc, transp->xp_netid,
		uaddr ? uaddr : "unknown");
	if (uaddr)
		free((void *) uaddr);
#endif
	/*
	 * Disallow calling rpcbind for certain procedures.
	 * Luckily Portmap set/unset/callit also have same procedure numbers.
	 * So, will not check for those.
	 */
	if (a.rmt_prog == RPCBPROG) {
		if ((a.rmt_proc == RPCBPROC_SET) ||
			(a.rmt_proc == RPCBPROC_UNSET) ||
			(a.rmt_proc == RPCBPROC_CALLIT)) {
			if (reply_type == RPCBPROC_INDIRECT)
				svcerr_weakauth(transp); /* XXX */
			if (debugging) {
				pfmt(stderr, MM_ERROR,
		     ":11:Calling %s procedures %s, %s, or %s is not allowed\n",
				     "RPCBPROG", "SET", "UNSET", "CALLIT");
			}
			goto error;
		}
		/*
		 * Ideally, we should have called rpcb_service() or
		 * pmap_service() with appropriate parameters instead of
		 * going about in a roundabout manner.  Hopefully,
		 * this case should happen rarely.
		 */
	}
	rbl = find_service(a.rmt_prog, a.rmt_vers, transp->xp_netid);

	rpcbs_rmtcall(versnum - 2, reply_type, a.rmt_prog, a.rmt_vers,
			a.rmt_proc, transp->xp_netid, rbl);

	if (rbl == (rpcblist_ptr)NULL) {
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "not found\n");
#endif
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_noprog(transp);
		goto error;
	}
	if (rbl->rpcb_map.r_vers != a.rmt_vers) {
#ifdef RPCBIND_DEBUG
		fprintf(stderr, "version not found\n");
#endif
		if (reply_type == RPCBPROC_INDIRECT) {
			u_long vers_low, vers_high;

			find_versions(a.rmt_prog, transp->xp_netid,
				&vers_low, &vers_high);
			svcerr_progvers(transp, vers_low, vers_high);
		}
		goto error;
	}

#ifdef RPCBIND_DEBUG
	fprintf(stderr, "found at uaddr %s\n", rbl->rpcb_map.r_addr);
#endif
	/*
	 *	Check whether this entry is valid and a server is present
	 *	Mergeaddr() returns NULL if no such entry is present, and
	 *	returns "" if the entry was present but the server is not
	 *	present (i.e., it crashed).
	 */
	uaddr = mergeaddr(transp, transp->xp_netid, rbl->rpcb_map.r_addr,
			NULL);
	if ((uaddr == (char *) NULL) || uaddr[0] == '\0') {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_noprog(transp);
		goto error;
	}

	nconf = rpcbind_get_conf(transp->xp_netid);
	if (nconf == (struct netconfig *)NULL) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			     ":22:Could not get %s structure for %s\n",
			     "netconfig", transp->xp_netid);
		}
		free((void *) uaddr);
		goto error;
	}
	if (uname(&utsname) == -1) {
		free((void *) uaddr);
		goto error;
	}
	hs.h_host = utsname.nodename;
	/* XXX */
	hs.h_serv = "rpcbind";
	if (netdir_getbyname(nconf, &hs, &nas) != ND_OK) {
		free((void *) uaddr);
		goto error;
	}
	ma.c_uaddr = taddr2uaddr(nconf, nas->n_addrs);
	ma.s_uaddr = rbl->rpcb_map.r_addr;
	/*
	 *	A mergeaddr operation allocates a string, which it stores in
	 *	ma.m_uaddr.  It's passed to forward_register() and is
	 *	eventually freed by free_slot_*().
	 */
	stat = netdir_options(nconf, ND_MERGEADDR, 0, (char *)&ma);
	free((void *) ma.c_uaddr);
	netdir_free((char *) nas, ND_ADDRLIST);
	if (stat) {
		(void)strcpy(syslogmsgp,
			     gettxt(":27",
				    "Could not merge addresses for %s: %s"));
		(void)syslog(LOG_ERR, syslogmsg,
			     nconf->nc_netid, netdir_sperror());
	}
#ifdef ND_DEBUG
	fprintf(stderr, "rpcbproc_callit_com: uaddr = %s, merged uaddr = %s\n",
				uaddr, ma.m_uaddr);
#endif
	free((void *) uaddr);

	/*
	 * We go thru this hoopla of opening up clnt_call() because we want
	 * to be able to set the xid of our choice.
	 */
	set_t_errno(0);
	if ((fd = t_open(nconf->nc_device, O_RDWR, NULL)) == -1) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		if (debugging) {
			if (t_errno == TSYSERR) {
				pfmt(stderr, MM_ERROR,
				     ":29:Could not open %s: ",
				     nconf->nc_device);
				perror("");
			} else {
				pfmt(stderr, MM_ERROR,
				     ":29:Could not open %s: ",
				     nconf->nc_device);
				t_error("");
			}
		}
		free((void *) ma.m_uaddr);
		goto error;
	}
	if (t_bind(fd, (struct t_bind *) 0,
		(struct t_bind *) 0) == -1) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		if (debugging) {
			if (t_errno == TSYSERR) {
				pfmt(stderr, MM_ERROR,
			       ":17:Could not bind to file descriptor for %s: ",
				     nconf->nc_device);
				perror("");
			} else {
				pfmt(stderr, MM_ERROR,
			       ":17:Could not bind to file descriptor for %s: ",
				     nconf->nc_device);
				t_error("");
			}
		}
		free((void *) ma.m_uaddr);
		goto error;
	}
	bd = getbogus_data(transp);
	call_msg.rm_xid = forward_register(bd->su_xid,
			svc_getrpccaller(transp), fd, ma.m_uaddr,
			reply_type, versnum);
	if (call_msg.rm_xid == 0) {
		/*
		 * A duplicate request for the slow server.  Let's not
		 * beat on it any more.
		 */
		if (debugging) {
			pfmt(stderr, MM_INFO, ":42:Duplicate request\n");
		}
		free((void *) ma.m_uaddr);
		goto error;
	} else 	if (call_msg.rm_xid == -1) {
		/*  forward_register failed.  Perhaps no memory. */
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			":6:%s failed\n", "forward_register");
		}
		free((void *) ma.m_uaddr);
		goto error;
	}

#ifdef RPCBIND_DEBUG
	fprintf(stderr,
		"rpcbproc_callit_com:  original XID %x, new XID %x\n",
			bd->su_xid, call_msg.rm_xid);
#endif
	call_msg.rm_direction = CALL;
	call_msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	call_msg.rm_call.cb_prog = a.rmt_prog;
	call_msg.rm_call.cb_vers = a.rmt_vers;
	if (sendsz > RPC_BUF_MAX) {
#ifdef	notyet
		outbuf_alloc = alloca(sendsz);	/* not in IDR2? */
#else
		outbuf_alloc = malloc(sendsz);
#endif	/* notyet */
		if (outbuf_alloc == NULL) {
			if (reply_type == RPCBPROC_INDIRECT)
				svcerr_systemerr(transp);
			if (debugging) {
				pfmt(stderr, MM_ERROR, ":56:No memory!");
				fprintf(stderr, "\n");
			}
			goto error;
		}
		xdrmem_create(&outxdr, outbuf_alloc, sendsz, XDR_ENCODE);
	} else {
		xdrmem_create(&outxdr, outbuf, sendsz, XDR_ENCODE);
	}
	if (!xdr_callhdr(&outxdr, &call_msg)) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			":6:%s failed\n", "xdr_callhdr");
		}
		goto error;
	}
	if (!xdr_u_long(&outxdr, &(a.rmt_proc))) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		if (debugging) {
			pfmt(stderr, MM_ERROR, ":6:%s failed\n", "xdr_u_long");
		}
		goto error;
	}

	if (rqstp->rq_cred.oa_flavor == AUTH_NULL) {
		auth = authnone_create();
	} else if (rqstp->rq_cred.oa_flavor == AUTH_SYS) {
		struct authsys_parms *au;

		au = (struct authsys_parms *)rqstp->rq_clntcred;
		auth = authsys_create(au->aup_machname,
				au->aup_uid, au->aup_gid,
				au->aup_len, au->aup_gids);
		if (auth == NULL) /* fall back */
			auth = authnone_create();
	} else {
		/* we do not support any other authentication scheme */
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			     ":1:%s != %s and %s != %s\n",
			     "oa_flavor", "AUTH_NONE", "oa_flavor", "AUTH_SYS");
		}
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_weakauth(transp); /* XXX too strong.. */
		goto error;
	}
	if (auth == NULL) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			   ":46:Failed to create authentication credentials\n");
		}
		goto error;
	}
	if (!AUTH_MARSHALL(auth, &outxdr)) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		AUTH_DESTROY(auth);
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			 ":26:Could not marshall authentication credentials\n");
		}
		goto error;
	}
	AUTH_DESTROY(auth);
	if (!xdr_opaque_parms(&outxdr, &a)) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			     ":6:%s failed\n", "xdr_opaque_parms");
		}
		goto error;
	}
	outlen = (int) XDR_GETPOS(&outxdr);
	if (outbuf_alloc)
		tu_data.udata.buf = outbuf_alloc;
	else
		tu_data.udata.buf = outbuf;
	tu_data.udata.len = outlen;
	tu_data.opt.len = 0;

	na = uaddr2taddr(nconf, ma.m_uaddr);
	if (!na) {
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		goto error;
	}
	tu_data.addr = *na;

	if (t_sndudata(fd, &tu_data) == -1) {
		if (debugging) {
			if (t_errno == TSYSERR) {
				pfmt(stderr, MM_ERROR,
				     ":5:%s failed: ", "t_sndudata");
				perror("");
			} else {
				pfmt(stderr, MM_ERROR,
				     ":5:%s failed: ", "t_sndudata");
				t_error("");
			}
		}
		if (reply_type == RPCBPROC_INDIRECT)
			svcerr_systemerr(transp);
		goto error;
	}
	goto out;

error:
	if (call_msg.rm_xid != 0)
		/* fd is closed here */
		(void) free_slot_by_xid(call_msg.rm_xid);
	else if (fd != -1)
		t_close(fd);
out:
	if (buf_alloc)
		free((void *) buf_alloc);
	if (outbuf_alloc)
		free((void *) outbuf_alloc);
	if (na)
		netdir_free((char *) na, ND_ADDR);
}

#define	NFORWARD	64
#define	MAXTIME_OFF	300	/* 5 minutes */

struct finfo {
	int		flag;
#define	FINFO_ACTIVE	0x1
	u_long		caller_xid;
	struct netbuf	*caller_addr;
	u_long		forward_xid;
	int		forward_fd;
	char		*uaddr;
	u_long		reply_type;
	u_long		versnum;
	time_t		time;
};
static struct finfo	FINFO[NFORWARD];
/*
 * Makes an entry into the FIFO for the given request.
 * If duplicate request, returns a 0, else returns the xid of its call.
 */
static u_long
forward_register(caller_xid, caller_addr, forward_fd, uaddr,
	reply_type, versnum)
	u_long		caller_xid;
	struct netbuf	*caller_addr;
	int		forward_fd;
	char		*uaddr;
	u_long		reply_type;
	u_long		versnum;
{
	int		i;
	int		j = 0;
	time_t		min_time, time_now;
	static u_long	lastxid;
	int		entry = -1;

	min_time = FINFO[0].time;
	time_now = time((time_t *)0);
	/* initialization */
	if (lastxid == 0)
		lastxid = time_now * NFORWARD;

	/*
	 * Check if it is an duplicate entry. Then,
	 * try to find an empty slot.  If not available, then
	 * use the slot with the earliest time.
	 */
	for (i = 0; i < NFORWARD; i++) {
		if (FINFO[i].flag & FINFO_ACTIVE) {
			if ((FINFO[i].caller_xid == caller_xid) &&
			    (FINFO[i].reply_type == reply_type) &&
			    (FINFO[i].versnum == versnum) &&
			    (!netbufcmp(FINFO[i].caller_addr,
					    caller_addr))) {
				FINFO[i].time = time((time_t *)0);
				return (0);	/* Duplicate entry */
			} else {
				/* Should we wait any longer */
				if ((time_now - FINFO[i].time) > MAXTIME_OFF)
					(void) free_slot_by_index(i);
			}
		}
		if (entry == -1) {
			if ((FINFO[i].flag & FINFO_ACTIVE) == 0) {
				entry = i;
			} else if (FINFO[i].time < min_time) {
				j = i;
				min_time = FINFO[i].time;
			}
		}
	}
	if (entry != -1) {
		/* use this empty slot */
		j = entry;
	} else {
		(void) free_slot_by_index(j);
	}
	if ((FINFO[j].caller_addr = netbufdup(caller_addr)) == NULL) {
		return (-1);
	}
	rpcb_rmtcalls++;	/* no of pending calls */
	FINFO[j].flag = FINFO_ACTIVE;
	FINFO[j].reply_type = reply_type;
	FINFO[j].versnum = versnum;
	FINFO[j].time = time_now;
	FINFO[j].caller_xid = caller_xid;
	FINFO[j].forward_fd = forward_fd;
	FD_SET(forward_fd, &svc_fdset);
	/*
	 * Though uaddr is not allocated here, it will still be freed
	 * from free_slot_*().
	 */
	FINFO[j].uaddr = uaddr;
	lastxid = lastxid + NFORWARD;
	FINFO[j].forward_xid = lastxid + j;	/* encode slot */
	return (FINFO[j].forward_xid);		/* forward on this xid */
}

static struct finfo *
forward_find(reply_xid)
	u_long		reply_xid;
{
	int		i;

	i = reply_xid % NFORWARD;
	if (i < 0)
		i += NFORWARD;
	if ((FINFO[i].flag & FINFO_ACTIVE) &&
	    (FINFO[i].forward_xid == reply_xid)) {
		return (&FINFO[i]);
	}
	return (NULL);
}

static int
free_slot_by_xid(xid)
	u_long xid;
{
	int entry;

	entry = xid % NFORWARD;
	if (entry < 0)
		entry += NFORWARD;
	return (free_slot_by_index(entry));
}

static int
free_slot_by_fd(fd)
	int fd;
{
	int i;

	for (i = 0; i < NFORWARD; i++)
		if ((FINFO[i].forward_fd == fd) &&
			(FINFO[i].flag & FINFO_ACTIVE))
			break;
	if (i == NFORWARD)
		return (0);
	return (free_slot_by_index(i));
}

static struct finfo *
find_fifo_by_fd(fd)
	int fd;
{
	int i;

	for (i = 0; i < NFORWARD; i++)
		if ((FINFO[i].forward_fd == fd) &&
			(FINFO[i].flag & FINFO_ACTIVE))
			break;
	if (i == NFORWARD)
		return (NULL);
	return (&FINFO[i]);
}

static int
free_slot_by_index(index)
	int index;
{
	struct finfo	*fi;

	fi = &FINFO[index];
	if (fi->flag & FINFO_ACTIVE) {
		netbuffree(fi->caller_addr);
		free((void *) fi->uaddr);
		if (fi->forward_fd != -1) {
			t_close(fi->forward_fd);
			FD_CLR(fi->forward_fd, &svc_fdset);
		}
		fi->flag &= ~FINFO_ACTIVE;
		rpcb_rmtcalls--;
		return (1);
	}
	return (0);
}

static int
netbufcmp(n1, n2)
	struct netbuf	*n1, *n2;
{
	return ((n1->len != n2->len) || memcmp(n1->buf, n2->buf, n1->len));
}

static struct netbuf *
netbufdup(ap)
	register struct netbuf  *ap;
{
	register struct netbuf  *np;

	np = (struct netbuf *) malloc(sizeof (struct netbuf) + ap->len);
	if (np) {
		np->maxlen = np->len = ap->len;
		np->buf = ((char *) np) + sizeof (struct netbuf);
		(void) memcpy(np->buf, ap->buf, ap->len);
	}
	return (np);
}

static void
netbuffree(ap)
	register struct netbuf  *ap;
{
	free((void *) ap);
}


#define	MASKVAL	(POLLIN | POLLPRI | POLLRDNORM | POLLRDBAND)

void
my_svc_run()
{
	size_t nfds;
	int dtbsize = _rpc_dtbsize();
	struct pollfd pollfds[FD_SETSIZE];
	int poll_ret, check_ret;
#ifdef SVC_RUN_DEBUG
	int i;
#endif
	register struct pollfd	*p;

	for (;;) {
		{
			register long *in;
			register int j;		/* loop counter */
			register u_long b;	/* bits to test */
			register int n;		/* loop counter */

			/*
			 * For each fd, if the appropriate bit is set
			 * convert it into the appropriate pollfd struct.
			 */
			p = pollfds;
			for (in = svc_fdset.fds_bits, n = 0; n < dtbsize;
					n += NFDBITS, in++) {
				for (b = (u_long) *in, j = 0; b; j++, b >>= 1)
					if (b & 1) {
						p->fd = n + j;
						if (p->fd >= NFDBITS)
							break;
						p->events = MASKVAL;
						p++;
					}
			}
			nfds = p - pollfds;
		}
		poll_ret = 0;
#ifdef SVC_RUN_DEBUG
		if (debugging) {
			fprintf(stderr, "polling for read on fd < ");
			for (i = 0, p = pollfds; i < nfds; i++, p++)
				if (p->events)
					fprintf(stderr, "%d ", p->fd);
			fprintf(stderr, ">\n");
		}
#endif
		switch (poll_ret = poll(pollfds, nfds, INFTIM)) {
		case -1:
			/*
			 * We ignore all errors, continuing with the assumption
			 * that it was set by the signal handlers (or any
			 * other outside event) and not caused by poll().
			 */
		case 0:
			continue;
		default:
#ifdef SVC_RUN_DEBUG
			if (debugging) {
				fprintf(stderr, "poll returned read fds < ");
				for (i = 0, p = pollfds; i < nfds; i++, p++)
					if (p->revents)
						fprintf(stderr, "%d ", p->fd);
				fprintf(stderr, ">\n");
			}
#endif
			/*
			 * If we found as many replies on callback fds
			 * as the number of descriptors selectable which
			 * poll() returned, there can be no more so we
			 * don't call svc_getreq_poll.  Otherwise, there
			 * must be another so we must call svc_getreq_poll.
			 */
			if ((check_ret = check_rmtcalls(pollfds, nfds)) ==
			    poll_ret)
				continue;
			svc_getreq_poll(pollfds, poll_ret-check_ret);
		}
	}
}

static int
check_rmtcalls(pfds, nfds)
	struct pollfd *pfds;
	int nfds;
{
	int i, j, ncallbacks_found, rmtcalls_pending;

	if (rpcb_rmtcalls == 0)
		return (0);
	for (i = 0, ncallbacks_found = 0, rmtcalls_pending = rpcb_rmtcalls;
		(ncallbacks_found < rmtcalls_pending) && (i < NFORWARD); i++) {
		register struct finfo	*fi = &FINFO[i];

		if (fi->flag & FINFO_ACTIVE) {
			for (j = 0; j < nfds; j++) {
				if (pfds[j].fd == fi->forward_fd) {
					if (pfds[j].revents) {
						ncallbacks_found++;
#ifdef SVC_RUN_DEBUG
			if (debugging)
				fprintf(stderr,
"svc_run:  polled on forwarding fd %d - calling handle_reply\n",
					fi->forward_fd);
#endif
						handle_reply(fi->forward_fd);
						pfds[j].revents = 0;
						break;
					}
				}
			}
		}
	}
	return (ncallbacks_found);
}

static void
xprt_set_caller(xprt, fi)
	SVCXPRT *xprt;
	struct finfo *fi;
{
	struct svc_dg_data *bd;

	*(svc_getrpccaller(xprt)) = *(fi->caller_addr);
	bd = (struct svc_dg_data *)getbogus_data(xprt);
	bd->su_xid = fi->caller_xid;	/* set xid on reply */
}

/*
 * Call svcerr_systemerr() only if RPCBVERS4
 */
static void
send_svcsyserr(xprt, fd)
	SVCXPRT *xprt;
	int fd;
{
	struct finfo	*fi;

	if ((fi = find_fifo_by_fd(fd)) == NULL) {
		if (debugging) {
			pfmt(stderr, MM_ERROR, ":51:Illegal file descriptor\n");
		}
		return;
	}
	if (fi->reply_type == RPCBPROC_INDIRECT) {
		xprt_set_caller(xprt, fi);
		svcerr_systemerr(xprt);
	}
	return;
}

static void
handle_reply(fd)
	int	fd;
{
	XDR		reply_xdrs;
	struct rpc_msg	reply_msg;
	struct rpc_err	reply_error;
	char		*buffer;
	extern int	errno, t_errno;
	SVCXPRT		*xprt;
	struct finfo	*fi;
	int		inlen, pos, len, res;
	struct r_rmtcall_args a;
	struct t_unitdata	*tr_data = NULL;
#ifdef RPCBIND_DEBUG
	char *uaddr;
#endif

	xprt = svc_tli_create(fd, 0, (struct t_bind *) 0, 0, 0);
	if (xprt == NULL) {
		/* NO error message can be sent back to the client */
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			     ":6:%s failed\n", "svc_tli_create");
		}
		(void) free_slot_by_fd(fd);
		return;
	}
	reply_msg.rm_xid = 0;  /* for easier error handling */
	tr_data = (struct t_unitdata *) t_alloc(fd, T_UNITDATA,
						T_ADDR | T_UDATA);
	if (tr_data == (struct t_unitdata *) NULL) {
		if (debugging) {
			pfmt(stderr, MM_ERROR,
			     ":6:%s failed\n", "t_alloc T_UNITDATA");
		}
		send_svcsyserr(xprt, fd);
		goto done;
	}
	do {
		int	moreflag;

		moreflag = 0;
		if (errno == EINTR)
			errno = 0;
		res = t_rcvudata(fd, tr_data, &moreflag);
		if (moreflag & T_MORE) {
			/* Drop this packet - we have no more space. */
			if (debugging) {
				pfmt(stderr, MM_WARNING,
				     ":63:Received packet with %s flag set\n",
				     "T_MORE");
			}
			send_svcsyserr(xprt, fd);
			goto done;
		}
	} while (res < 0 && errno == EINTR);
	if (res < 0) {
		if (t_errno == TLOOK) {
			if (debugging) {
				pfmt(stderr, MM_WARNING,
				     ":7:%s returned %d, %s\n", "t_rcvudata",
				     res, "t_errno TLOOK");
			}
			(void) t_rcvuderr(fd, (struct t_uderr *) NULL);
		}
#ifdef RPCBIND_DEBUG
		if (debugging)
			fprintf(stderr,
	"handle_reply:  t_rcvudata returned %d, t_errno %d, errno %d\n",
				res, t_errno, errno);
#endif
		send_svcsyserr(xprt, fd);
		goto done;
	}
	inlen = tr_data->udata.len;
#ifdef DEBUG_MORE
	if (debugging)
		fprintf(stderr,
		"handle_reply:  t_rcvudata received %d-byte packet from %s\n",
		inlen, taddr2uaddr(rpcbind_get_conf("udp"), &tr_data->addr));
#endif
	buffer = tr_data->udata.buf;
	if (buffer == (char *) NULL) {
		send_svcsyserr(xprt, fd);
		goto done;
	}

	reply_msg.acpted_rply.ar_verf = _null_auth;
	reply_msg.acpted_rply.ar_results.where = 0;
	reply_msg.acpted_rply.ar_results.proc = (xdrproc_t) xdr_void;

	xdrmem_create(&reply_xdrs, buffer, (u_int)inlen, XDR_DECODE);
	if (!xdr_replymsg(&reply_xdrs, &reply_msg)) {
		if (debugging) {
			pfmt(stderr, MM_ERROR, ":6:%s failed\n", "xdr_replymsg");
		}
		send_svcsyserr(xprt, fd);
		goto done;
	}
	_seterr_reply(&reply_msg, &reply_error);
	if (reply_error.re_status != RPC_SUCCESS) {
		if (debugging) {
			pfmt(stderr, MM_ERROR,
		             ":44:Error in reply: %s\n",
				clnt_sperrno(reply_error.re_status));
		}
		send_svcsyserr(xprt, fd);
		goto done;
	}
	fi = forward_find(reply_msg.rm_xid);
	if (fi == NULL) {
		send_svcsyserr(xprt, fd);
		goto done;
	}
	pos = XDR_GETPOS(&reply_xdrs);
	len = inlen - pos;
	a.rmt_args.args = &buffer[pos];
	a.rmt_args.arglen = len;
	a.rmt_uaddr = fi->uaddr;
	a.rmt_localvers = fi->versnum;

	xprt_set_caller(xprt, fi);
#ifdef RPCBIND_DEBUG
	uaddr =	taddr2uaddr(rpcbind_get_conf("udp"),
				    svc_getrpccaller(xprt));
	fprintf(stderr,
		"handle_reply:  forwarding address %s to %s\n",
		a.rmt_uaddr, uaddr ? uaddr : "unknown");
	if (uaddr)
		free((void *) uaddr);
#endif
	svc_sendreply(xprt, (xdrproc_t) xdr_rmtcall_result, (char *) &a);
done:
	xprt->xp_rtaddr.buf = NULL;
	SVC_DESTROY(xprt); /* Note: fd gets destroyed here also */
	if (tr_data)
		t_free((char *) tr_data, T_UNITDATA);
	if (reply_msg.rm_xid == 0)
		(void) free_slot_by_fd(fd);
	else
		(void) free_slot_by_xid(reply_msg.rm_xid);
	return;
}

static void
find_versions(prog, netid, lowvp, highvp)
	u_long prog;	/* Program Number */
	char *netid;	/* Transport Provider token */
	u_long *lowvp;  /* Low version number */
	u_long *highvp; /* High version number */
{
	register rpcblist_ptr rbl;
	int lowv = 0;
	int highv = 0;

	for (rbl = list_rbl; rbl != NULL; rbl = rbl->rpcb_next) {
		if ((rbl->rpcb_map.r_prog != prog) ||
		    ((rbl->rpcb_map.r_netid != NULL) &&
			(strcasecmp(rbl->rpcb_map.r_netid, netid) != 0)))
			continue;
		if (lowv == 0) {
			highv = rbl->rpcb_map.r_vers;
			lowv = highv;
		} else if (rbl->rpcb_map.r_vers < lowv) {
			lowv = rbl->rpcb_map.r_vers;
		} else if (rbl->rpcb_map.r_vers > highv) {
			highv = rbl->rpcb_map.r_vers;
		}
	}
	*lowvp = lowv;
	*highvp = highv;
	return;
}

/*
 * returns the item with the given program, version number and netid.
 * If that version number is not found, it returns the item with that
 * program number, so that address is now returned to the caller. The
 * caller when makes a call to this program, version number, the call
 * will fail and it will return with PROGVERS_MISMATCH. The user can
 * then determine the highest and the lowest version number for this
 * program using clnt_geterr() and use those program version numbers.
 *
 * Returns the RPCBLIST for the given prog, vers and netid
 */
static rpcblist_ptr
find_service(prog, vers, netid)
	u_long prog;	/* Program Number */
	u_long vers;	/* Version Number */
	char *netid;	/* Transport Provider token */
{
	register rpcblist_ptr hit = NULL;
	register rpcblist_ptr rbl;

	for (rbl = list_rbl; rbl != NULL; rbl = rbl->rpcb_next) {
		if ((rbl->rpcb_map.r_prog != prog) ||
		    ((rbl->rpcb_map.r_netid != NULL) &&
			(strcasecmp(rbl->rpcb_map.r_netid, netid) != 0)))
			continue;
		hit = rbl;
		if (rbl->rpcb_map.r_vers == vers)
			break;
	}
	return (hit);
}

/*
 * Copies the name associated with the uid of the caller and returns
 * a pointer to it.  Similar to getwd().
 */
static char *
getowner(transp, owner)
	SVCXPRT *transp;
	char *owner;
{
	uid_t uid;

	if (_rpc_get_local_uid(transp, &uid) < 0)
		return (strcpy(owner, "unknown"));
	if (uid == 0)
		return (strcpy(owner, "superuser"));
	(void) sprintf(owner, "%u", uid);
	return (owner);
}

#ifdef PORTMAP
/*
 * Add this to the pmap list only if it is UDP or TCP.
 */
static int
add_pmaplist(arg)
	RPCB *arg;
{
	pmap pmap;
	pmaplist *pml;
	int h1, h2, h3, h4, p1, p2;

	if (strcmp(arg->r_netid, udptrans) == 0) {
		/* It is UDP! */
		pmap.pm_prot = IPPROTO_UDP;
	} else if (strcmp(arg->r_netid, tcptrans) == 0) {
		/* It is TCP */
		pmap.pm_prot = IPPROTO_TCP;
	} else
		/* Not a IP protocol */
		return (0);

	/* interpret the universal address for TCP/IP */
	if (sscanf(arg->r_addr, "%d.%d.%d.%d.%d.%d",
		&h1, &h2, &h3, &h4, &p1, &p2) != 6)
		return (0);
	pmap.pm_port = ((p1 & 0xff) << 8) + (p2 & 0xff);
	pmap.pm_prog = arg->r_prog;
	pmap.pm_vers = arg->r_vers;
	/*
	 * add to END of list
	 */
	pml = (pmaplist *) malloc((u_int)sizeof (pmaplist));
	if (pml == NULL) {
		(void)strcpy(syslogmsgp,
			     gettxt(":56", "No memory!"));
		(void) syslog(LOG_ERR, syslogmsg);
		return (1);
	}
	pml->pml_map = pmap;
	pml->pml_next = NULL;
	if (list_pml == NULL) {
		list_pml = pml;
	} else {
		pmaplist *fnd;

		/* Attach to the end of the list */
		for (fnd = list_pml; fnd->pml_next; fnd = fnd->pml_next)
			;
		fnd->pml_next = pml;
	}
	return (0);
}

/*
 * Delete this from the pmap list only if it is UDP or TCP.
 */
static int
del_pmaplist(arg)
	RPCB *arg;
{
	register pmaplist *pml;
	pmaplist *prevpml, *fnd;
	long prot;

	if (strcmp(arg->r_netid, udptrans) == 0) {
		/* It is UDP! */
		prot = IPPROTO_UDP;
	} else if (strcmp(arg->r_netid, tcptrans) == 0) {
		/* It is TCP */
		prot = IPPROTO_TCP;
	} else if (arg->r_netid[0] == NULL) {
		prot = 0;	/* Remove all occurrences */
	} else {
		/* Not a IP protocol */
		return (0);
	}
	for (prevpml = NULL, pml = list_pml; pml; ) {
		if ((pml->pml_map.pm_prog != arg->r_prog) ||
			(pml->pml_map.pm_vers != arg->r_vers) ||
			(prot && (pml->pml_map.pm_prot != prot))) {
			/* both pml & prevpml move forwards */
			prevpml = pml;
			pml = pml->pml_next;
			continue;
		}
		/* found it; pml moves forward, prevpml stays */
		fnd = pml;
		pml = pml->pml_next;
		if (prevpml == NULL)
			list_pml = pml;
		else
			prevpml->pml_next = pml;
		free((void *) fnd);
	}
	return (0);
}
#endif /* PORTMAP */

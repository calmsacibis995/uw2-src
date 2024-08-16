/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/svc.c	1.4.12.4"
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
 * svc.c, Server-side remote procedure call interface.
 *
 * There are two sets of procedures here.  The xprt routines are
 * for handling transport handles.  The svc routines handle the
 * list of service routines.
 *
 */

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include "trace.h"
#include <rpc/rpc.h>
#ifdef PORTMAP
#include <rpc/pmap_clnt.h>
#endif
#include <sys/poll.h>
#include <netconfig.h>
#include <sys/syslog.h>
#include <stdlib.h>
#ifdef _REENTRANT
#include <thread.h>
#include <stropts.h>
#endif /* _REENTRANT */
#include "rpc_mt.h"

static SVCXPRT **xports;

#define	NULL_SVC ((struct svc_callout *)0)
#define	RQCRED_SIZE	400		/* this size is excessive */

#define	SVC_VERSQUIET 0x0001		/* keep quiet about vers mismatch */
#define	version_keepquiet(xp)  ((u_long)(xp)->xp_p3 & SVC_VERSQUIET)

/*
 * The services list
 * Each entry represents a set of procedures (an rpc program).
 * The dispatch routine takes request structs and runs the
 * appropriate procedure.
 */
static struct svc_callout {
	struct svc_callout *sc_next;
	u_long		    sc_prog;
	u_long		    sc_vers;
	char		   *sc_netid;
	void		    (*sc_dispatch)();
} *svc_head;

static struct svc_callout *svc_find();
static void prog_dispatch();
void svc_getreq_common();
char *strdup();

/* ***************  SVCXPRT related stuff **************** */

/*
 * Activate a transport handle.
 */
void
xprt_register(xprt)
	SVCXPRT *xprt;
{
	register int fd = xprt->xp_fd;
#ifdef CALLBACK
	extern void (*_svc_getreqset_proc)();
#endif

	trace1(TR_xprt_register, 0);
	MUTEX_LOCK(&__svc_lock);
	if (xports == NULL) {
		xports = (SVCXPRT **)
			mem_alloc((FD_SETSIZE + 1)* sizeof (SVCXPRT *));
#ifdef CALLBACK
		/*
		 * XXX: This code does not keep track of the server state.
		 *
		 * This provides for callback support.	When a client
		 * recv's a call from another client on the server fd's,
		 * it calls _svc_getreqset_proc() which would return
		 * after serving all the server requests.  Also look under
		 * clnt_dg.c and clnt_vc.c  (clnt_call part of it)
		 */
		_svc_getreqset_proc = svc_getreq_poll;
#endif
	}
	if (fd < FD_SETSIZE +1)
		xports[fd] = xprt;
	if (fd < _rpc_dtbsize())
		FD_SET(fd, &svc_fdset);
	MUTEX_UNLOCK(&__svc_lock);

	trace1(TR_xprt_register, 1);
}

/*
 * De-activate a transport handle.
 */
void
xprt_unregister(xprt)
	SVCXPRT *xprt;
{
	register int fd = xprt->xp_fd;

	trace1(TR_xprt_unregister, 0);
	MUTEX_LOCK(&__svc_lock);
	if ((fd < _rpc_dtbsize()) && (xports[fd] == xprt)) {
		xports[fd] = (SVCXPRT *)NULL;
		FD_CLR(fd, &svc_fdset);
	}
	MUTEX_UNLOCK(&__svc_lock);

	trace1(TR_xprt_unregister, 1);
}


/* ********************** CALLOUT list related stuff ************* */

/*
 * Add a service program to the callout list.
 * The dispatch routine will be called when a rpc request for this
 * program number comes in.
 */
bool_t
svc_reg(xprt, prog, vers, dispatch, nconf)
	SVCXPRT *xprt;
	u_long prog;
	u_long vers;
	void (*dispatch)();
	struct netconfig *nconf;
{
	bool_t dummy;
	struct svc_callout *prev;
	register struct svc_callout *s;
	struct netconfig *tnconf;
	register char *netid = NULL;
	int flag = 0;

	trace3(TR_svc_reg, 0, prog, vers);
	if (xprt->xp_netid) {
		netid = strdup(xprt->xp_netid);
		flag = 1;
	} else if (nconf && nconf->nc_netid) {
		netid = strdup(nconf->nc_netid);
		flag = 1;
	} else if ((tnconf = _rpcgettp(xprt->xp_fd)) != NULL) {
		netid = strdup(tnconf->nc_netid);
		flag = 1;
		freenetconfigent(tnconf);
	} /* must have been created with svc_raw_create */
	if ((netid == NULL) && (flag == 1)) {
		trace3(TR_svc_reg, 1, prog, vers);
		return (FALSE);
	}
	/*
	 * _svc_lock is held until a new entry is added to the head.
	 */
	MUTEX_LOCK(&__svc_lock);
	if ((s = svc_find(prog, vers, &prev, netid)) != NULL_SVC) {
		MUTEX_UNLOCK(&__svc_lock);

		if (netid)
			free(netid);
		if (s->sc_dispatch == dispatch)
			goto rpcb_it; /* he is registering another xptr */
		trace3(TR_svc_reg, 1, prog, vers);
		return (FALSE);
	}
	s = (struct svc_callout *)mem_alloc(sizeof (struct svc_callout));
	if (s == (struct svc_callout *)NULL) {
		MUTEX_UNLOCK(&__svc_lock);

		if (netid)
			free(netid);
		trace3(TR_svc_reg, 1, prog, vers);
		return (FALSE);
	}

	s->sc_prog = prog;
	s->sc_vers = vers;
	s->sc_dispatch = dispatch;
	s->sc_netid = netid;
	s->sc_next = svc_head;
	svc_head = s;
	MUTEX_UNLOCK(&__svc_lock);

	if ((xprt->xp_netid == NULL) && (flag == 1) && netid)
		xprt->xp_netid = strdup(netid);

rpcb_it:
	/* now register the information with the local binder service */
	if (nconf) {
		dummy = rpcb_set(prog, vers, nconf, &xprt->xp_ltaddr);
		trace3(TR_svc_reg, 1, prog, vers);
		return (dummy);
	}
	trace3(TR_svc_reg, 1, prog, vers);
	return (TRUE);
}

/*
 * Remove a service program from the callout list.
 */
void
svc_unreg(prog, vers)
	u_long prog;
	u_long vers;
{
	struct svc_callout *prev;
	register struct svc_callout *s;

	trace3(TR_svc_unreg, 0, prog, vers);
	/* unregister the information anyway */
	(void) rpcb_unset(prog, vers, NULL);
	MUTEX_LOCK(&__svc_lock);
	while ((s = svc_find(prog, vers, &prev, NULL)) != NULL_SVC) {
		if (prev == NULL_SVC) {
			svc_head = s->sc_next;
		} else {
			prev->sc_next = s->sc_next;
		}
		s->sc_next = NULL_SVC;
		if (s->sc_netid)
			mem_free((char *)s->sc_netid,
					(u_int)sizeof (s->sc_netid) + 1);
		mem_free((char *)s, (u_int) sizeof (struct svc_callout));
	}
	MUTEX_UNLOCK(&__svc_lock);
	trace3(TR_svc_unreg, 1, prog, vers);
}

#ifdef PORTMAP
/*
 * Add a service program to the callout list.
 * The dispatch routine will be called when a rpc request for this
 * program number comes in.
 * For version 2 portmappers.
 */
bool_t
svc_register(xprt, prog, vers, dispatch, protocol)
	SVCXPRT *xprt;
	u_long prog;
	u_long vers;
	void (*dispatch)();
	int protocol;
{
	bool_t dummy;
	struct svc_callout *prev;
	register struct svc_callout *s;
	register struct netconfig *nconf;
	register char *netid = NULL;
	int flag = 0;

	trace4(TR_svc_register, 0, prog, vers, protocol);
	if (xprt->xp_netid) {
		netid = strdup(xprt->xp_netid);
		flag = 1;
	} else if ((nconf = _rpcgettp(xprt->xp_fd)) != NULL) {
		/* fill in missing netid field in SVCXPRT */
		netid = strdup(nconf->nc_netid);
		flag = 1;
		freenetconfigent(nconf);
	} /* must be svc_raw_create */

	if ((netid == NULL) && (flag == 1)) {
		trace4(TR_svc_register, 1, prog, vers, protocol);
		return (FALSE);
	}

	MUTEX_LOCK(&__svc_lock);
	if ((s = svc_find(prog, vers, &prev, netid)) != NULL_SVC) {
		MUTEX_UNLOCK(&__svc_lock);

		if (netid)
			free(netid);
		if (s->sc_dispatch == dispatch)
			goto pmap_it;  /* he is registering another xptr */
		trace4(TR_svc_register, 1, prog, vers, protocol);
		return (FALSE);
	}
	s = (struct svc_callout *)mem_alloc(sizeof (struct svc_callout));
	if (s == (struct svc_callout *)0) {
		MUTEX_UNLOCK(&__svc_lock);

		if (netid)
			free(netid);
		trace4(TR_svc_register, 1, prog, vers, protocol);
		return (FALSE);
	}
	s->sc_prog = prog;
	s->sc_vers = vers;
	s->sc_dispatch = dispatch;
	s->sc_netid = netid;
	s->sc_next = svc_head;
	svc_head = s;
	MUTEX_UNLOCK(&__svc_lock);

	if ((xprt->xp_netid == NULL) && (flag == 1) && netid)
		xprt->xp_netid = strdup(netid);

pmap_it:
	/* now register the information with the local binder service */
	if (protocol) {
		dummy = pmap_set(prog, vers, protocol, xprt->xp_port);
		trace4(TR_svc_register, 1, prog, vers, protocol);
		return (dummy);
	}
	trace4(TR_svc_register, 1, prog, vers, protocol);
	return (TRUE);
}

/*
 * Remove a service program from the callout list.
 * For version 2 portmappers.
 */
void
svc_unregister(prog, vers)
	u_long prog;
	u_long vers;
{
	struct svc_callout *prev;
	register struct svc_callout *s;

	trace3(TR_svc_unregister, 0, prog, vers);
	MUTEX_LOCK(&__svc_lock);
	while ((s = svc_find(prog, vers, &prev, NULL)) != NULL_SVC) {
		if (prev == NULL_SVC) {
			svc_head = s->sc_next;
		} else {
			prev->sc_next = s->sc_next;
		}
		s->sc_next = NULL_SVC;
		if (s->sc_netid)
			mem_free((char *)s->sc_netid,
					(u_int)sizeof (s->sc_netid) + 1);
		mem_free((char *) s, (u_int) sizeof (struct svc_callout));
		/* unregister the information with the local binder service */
		(void) pmap_unset(prog, vers);
	}
	MUTEX_UNLOCK(&__svc_lock);

	trace3(TR_svc_unregister, 1, prog, vers);
}

#endif /* PORTMAP */
/*
 * Search the callout list for a program number, return the callout
 * struct.
 * Also check for transport as well.  Many routines such as svc_unreg
 * dont give any corresponding transport, so dont check for transport if
 * netid == NULL
 */

/*
 * _svc_lock should be held before entering this function.
 */
static struct svc_callout *
svc_find(prog, vers, prev, netid)
	u_long prog;
	u_long vers;
	struct svc_callout **prev;
	char *netid;
{
	register struct svc_callout *s, *p;

	trace3(TR_svc_find, 0, prog, vers);
	p = NULL_SVC;
	for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
		if (((s->sc_prog == prog) && (s->sc_vers == vers)) &&
			((netid == NULL) || (s->sc_netid == NULL) ||
			(strcmp(netid, s->sc_netid) == 0)))
				break;
		p = s;
	}
	*prev = p;
	trace3(TR_svc_find, 1, prog, vers);
	return (s);
}


/* ******************* REPLY GENERATION ROUTINES  ************ */

/*
 * Send a reply to an rpc request
 */
bool_t
svc_sendreply(xprt, xdr_results, xdr_location)
	register SVCXPRT *xprt;
	xdrproc_t xdr_results;
	caddr_t xdr_location;
{
	bool_t dummy;
	struct rpc_msg rply;

	trace1(TR_svc_sendreply, 0);
	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = SUCCESS;
	rply.acpted_rply.ar_results.where = xdr_location;
	rply.acpted_rply.ar_results.proc = xdr_results;
	dummy = SVC_REPLY(xprt, &rply);
	trace1(TR_svc_sendreply, 1);
	return (dummy);
}

/*
 * No procedure error reply
 */
void
svcerr_noproc(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply;

	trace1(TR_svcerr_noproc, 0);
	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROC_UNAVAIL;
	SVC_REPLY(xprt, &rply);
	trace1(TR_svcerr_noproc, 1);
}

/*
 * Can't decode args error reply
 */
void
svcerr_decode(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply;

	trace1(TR_svcerr_decode, 0);
	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = GARBAGE_ARGS;
	SVC_REPLY(xprt, &rply);
	trace1(TR_svcerr_decode, 1);
}

/*
 * Some system error
 */
void
svcerr_systemerr(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply;

	trace1(TR_svcerr_systemerr, 0);
	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = SYSTEM_ERR;
	SVC_REPLY(xprt, &rply);
	trace1(TR_svcerr_systemerr, 1);
}

/*
 * Tell RPC package to not complain about version errors to the client.	 This
 * is useful when revving broadcast protocols that sit on a fixed address.
 * There is really one (or should be only one) example of this kind of
 * protocol: the portmapper (or rpc binder).
 */
void
svc_versquiet(xprt)
	register SVCXPRT *xprt;
{
	u_long tmp;

	trace1(TR_svc_versquiet, 0);
	tmp = ((u_long)xprt->xp_p3) | SVC_VERSQUIET;
	xprt->xp_p3 = (caddr_t)tmp;
	trace2(TR_svc_versquiet, 1, tmp);
}

/*
 * Authentication error reply
 */
void
svcerr_auth(xprt, why)
	SVCXPRT *xprt;
	enum auth_stat why;
{
	struct rpc_msg rply;

	trace1(TR_svcerr_auth, 0);
	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_DENIED;
	rply.rjcted_rply.rj_stat = AUTH_ERROR;
	rply.rjcted_rply.rj_why = why;
	SVC_REPLY(xprt, &rply);
	trace1(TR_svcerr_auth, 1);
}

/*
 * Auth too weak error reply
 */
void
svcerr_weakauth(xprt)
	SVCXPRT *xprt;
{
	trace1(TR_svcerr_weakauth, 0);
	svcerr_auth(xprt, AUTH_TOOWEAK);
	trace1(TR_svcerr_weakauth, 1);
}

/*
 * Program unavailable error reply
 */
void
svcerr_noprog(xprt)
	register SVCXPRT *xprt;
{
	struct rpc_msg rply;

	trace1(TR_svcerr_noprog, 0);
	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROG_UNAVAIL;
	SVC_REPLY(xprt, &rply);
	trace1(TR_svcerr_noprog, 1);
}

/*
 * Program version mismatch error reply
 */
void
svcerr_progvers(xprt, low_vers, high_vers)
	register SVCXPRT *xprt;
	u_long low_vers;
	u_long high_vers;
{
	struct rpc_msg rply;

	trace3(TR_svcerr_progvers, 0, low_vers, high_vers);
	rply.rm_direction = REPLY;
	rply.rm_reply.rp_stat = MSG_ACCEPTED;
	rply.acpted_rply.ar_verf = xprt->xp_verf;
	rply.acpted_rply.ar_stat = PROG_MISMATCH;
	rply.acpted_rply.ar_vers.low = low_vers;
	rply.acpted_rply.ar_vers.high = high_vers;
	SVC_REPLY(xprt, &rply);
	trace3(TR_svcerr_progvers, 1, low_vers, high_vers);
}

/* ******************* SERVER INPUT STUFF ******************* */

/*
 * Get server side input from some transport.
 *
 * Statement of authentication parameters management:
 * This function owns and manages all authentication parameters, specifically
 * the "raw" parameters (msg.rm_call.cb_cred and msg.rm_call.cb_verf) and
 * the "cooked" credentials (rqst->rq_clntcred).
 * However, this function does not know the structure of the cooked
 * credentials, so it make the following assumptions:
 *   a) the structure is contiguous (no pointers), and
 *   b) the cred structure size does not exceed RQCRED_SIZE bytes.
 * In all events, all three parameters are freed upon exit from this routine.
 * The storage is trivially management on the call stack in user land, but
 * is mallocated in kernel land.
 */

void
svc_getreq(rdfds)
	int rdfds;
{
	fd_set readfds;

	trace2(TR_svc_getreq, 0, rdfds);
	FD_ZERO(&readfds);
	readfds.fds_bits[0] = rdfds;
	svc_getreqset(&readfds);
	trace2(TR_svc_getreq, 1, rdfds);
}

void
svc_getreqset(readfds)
	fd_set *readfds;
{
	register u_long mask;
	register int bit;
	register u_long *maskp;
	register int setsize;
	register int i;

	trace1(TR_svc_getreqset, 0);
	setsize = _rpc_dtbsize();
	maskp = (u_long *)readfds->fds_bits;
	for (i = 0; i < setsize; i += NFDBITS)
		for (mask = *maskp++; bit = ffs(mask);
			mask ^= (1 << (bit - 1)))
			/* fd has input waiting */
			svc_getreq_common(i + bit - 1);
	trace1(TR_svc_getreqset, 1);
}

void
svc_getreq_poll(pfdp, pollretval)
	struct pollfd	*pfdp;
	int	pollretval;
{
	register int setsize;
	register int i;
	register int fds_found;

	trace2(TR_svc_getreq_poll, 0, pollretval);
	setsize = _rpc_dtbsize();
	for (i = fds_found = 0; i < setsize && fds_found < pollretval; i++) {
		register struct pollfd *p = &pfdp[i];

		if (p->revents) {
			/* fd has input waiting */
			fds_found++;
			/*
			 *	We assume that this function is only called
			 *	via someone select()ing from svc_fdset or
			 *	poll()ing from svc_pollset[].  Thus it's safe
			 *	to handle the POLLNVAL event by simply turning
			 *	the corresponding bit off in svc_fdset.  The
			 *	svc_pollset[] array is derived from svc_fdset
			 *	and so will also be updated eventually.
			 *
			 *	XXX Should we do an xprt_unregister() instead?
			 */
			if (p->revents & POLLNVAL) {
				MUTEX_LOCK(&__svc_lock);
				FD_CLR(p->fd, &svc_fdset);	/* XXX */
				MUTEX_UNLOCK(&__svc_lock);
			} else
				svc_getreq_common(p->fd);
		}
	}
	trace2(TR_svc_getreq_poll, 1, pollretval);
}

#ifdef _REENTRANT

static	int	*svc_workmap;

/*
 * BEWARE:
 * this function assumes that the caller did a MUTEX_LOCK(&__svc_lock)
 */

void
_svc_getreq_poll_parallel(pfdp, pollretval)
	struct pollfd	*pfdp;
	int	pollretval;
{
	register int setsize;
	register int i;
	register int fds_found;

	trace2(TR_svc_getreq_poll_parallel, 0, pollretval);
	if (!svc_workmap) {
		svc_workmap = calloc(FD_SETSIZE, sizeof(int));
		if (!svc_workmap) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:32", "%s: out of memory"),
			    "svc_getreq_poll_parallel");
			trace2(TR_svc_getreq_poll_parallel, 2,
			    pollretval);
			return;
		}
	}

	setsize = _rpc_dtbsize();
	for (i = fds_found = 0; i < setsize && fds_found < pollretval; i++) {
		register struct pollfd *p = &pfdp[i];

		if ((svc_workmap[i] == 0) && (p->revents)) {
			/*
			 * fd has input waiting
			 */
			fds_found++;
			/*
			 * We assume that this function is only called
			 * via someone select()ing from svc_fdset or
			 * poll()ing from svc_pollset[].  Thus it's safe
			 * to handle the POLLNVAL event by simply turning
			 * the corresponding bit off in svc_fdset.  The
			 * svc_pollset[] array is derived from svc_fdset
			 * and so will also be updated eventually.
			 * 
			 * XXX Should we do an xprt_unregister() instead?
			 */
			if (p->revents & POLLNVAL) {
				FD_CLR(p->fd, &svc_fdset);	/* XXX */
			} else {
				svc_workmap[i] = 1;
				MUTEX_UNLOCK(&__svc_lock);
				svc_getreq_common(p->fd);
				MUTEX_LOCK(&__svc_lock);
				svc_workmap[i] = 0;
			}
		}
	}
	trace2(TR_svc_getreq_poll_parallel, 1, pollretval);
}

/*
 * wrapper for _svc_getreq_poll_parallel(), so that users
 * can call it.
 */

void
svc_getreq_poll_parallel(pfdp, pollretval)
	struct pollfd	*pfdp;
	int	pollretval;
{
	MUTEX_LOCK(&__svc_lock);
	_svc_getreq_poll_parallel(pfdp, pollretval);
	MUTEX_UNLOCK(&__svc_lock);
}

#endif

void
svc_getreq_common(fd)
	int fd;
{
	register SVCXPRT *xprt = xports[fd];
	enum xprt_stat stat;
	struct rpc_msg msg;
	struct svc_req r;
	char cred_area[2 * MAX_AUTH_BYTES + RQCRED_SIZE];

	trace2(TR_svc_getreq_common, 0, fd);
	msg.rm_call.cb_cred.oa_base = cred_area;
	msg.rm_call.cb_verf.oa_base = &(cred_area[MAX_AUTH_BYTES]);
	r.rq_clntcred = &(cred_area[2 * MAX_AUTH_BYTES]);

	if (xprt == NULL) {
		syslog(LOG_ERR, 
		    gettxt("uxstr:90",
			"svc_getreqset: No transport handle for fd %d"),
		    fd);

		trace2(TR_svc_getreq_common, 1, fd);
		return;
	}

	/* receive msgs from xprtprt (support batch calls) */
	do {
		if (SVC_RECV(xprt, &msg)) {
			prog_dispatch(xprt, &msg, &r);
		}
		/*
		 * Check if the xprt has been disconnected in a recursive call
		 * in the service dispatch routine. If so, then break
		 */
		if (xprt != xports[fd])
			break;

		if ((stat = SVC_STAT(xprt)) == XPRT_DIED) {
			SVC_DESTROY(xprt);
			break;
		}
	} while (stat == XPRT_MOREREQS);
	trace2(TR_svc_getreq_common, 1, fd);
}

static void
prog_dispatch(xprt, msg, r)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
	struct svc_req *r;
{
	register struct svc_callout *s;
	enum auth_stat why;
	int prog_found;
	u_long low_vers;
	u_long high_vers;

	trace1(TR_prog_dispatch, 0);
	r->rq_xprt = xprt;
	r->rq_prog = msg->rm_call.cb_prog;
	r->rq_vers = msg->rm_call.cb_vers;
	r->rq_proc = msg->rm_call.cb_proc;
	r->rq_cred = msg->rm_call.cb_cred;

	/* first authenticate the message */
	if ((why = _authenticate(r, msg)) != AUTH_OK) {
		svcerr_auth(xprt, why);
		trace1(TR_prog_dispatch, 1);
		return;
	}
	/* match message with a registered service */
	prog_found = FALSE;
	low_vers = 0 - 1;
	high_vers = 0;
	MUTEX_LOCK(&__svc_lock);
	for (s = svc_head; s != NULL_SVC; s = s->sc_next) {
		if (s->sc_prog == r->rq_prog) {
			prog_found = TRUE;
			if (s->sc_vers == r->rq_vers) {
				if ((xprt->xp_netid == NULL) ||
				    (s->sc_netid == NULL) ||
				    (strcmp(xprt->xp_netid,
					    s->sc_netid) == 0)) {
#ifdef _REENTRANT
					void (*proc)();
					proc = s->sc_dispatch;
					MUTEX_UNLOCK(&__svc_lock);
					(*proc)(r, xprt);
#else
					(*s->sc_dispatch)(r, xprt);
#endif /* _REENTRANT */
					trace1(TR_prog_dispatch, 1);
					return;
				} else {
					prog_found = FALSE;
				}
			}
			if (s->sc_vers < low_vers)
				low_vers = s->sc_vers;
			if (s->sc_vers > high_vers)
				high_vers = s->sc_vers;
		}		/* found correct program */
	}
	MUTEX_UNLOCK(&__svc_lock);

	/*
	 * if we got here, the program or version
	 * is not served ...
	 */
	if (prog_found && !version_keepquiet(xprt))
		svcerr_progvers(xprt, low_vers, high_vers);
	else
		svcerr_noprog(xprt);
	trace1(TR_prog_dispatch, 1);
	return;
}

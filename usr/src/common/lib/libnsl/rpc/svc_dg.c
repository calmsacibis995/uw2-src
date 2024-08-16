/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/svc_dg.c	1.4.10.3"
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
 * svc_dg.c
 *
 * Server side for connectionless RPC.
 * Does some caching in the hopes of achieving execute-at-most-once semantics.
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include "trace.h"
#include <rpc/rpc.h>
#include <errno.h>
#include <sys/syslog.h>
#ifdef RPC_CACHE_DEBUG
#include <netconfig.h>
#include <netdir.h>
#endif

#ifndef MAX
#define	MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

static struct xp_ops *svc_dg_ops();

extern char *malloc();

#define	MAX_OPT_WORDS	32

/*
 * kept in xprt->xp_p2
 */
struct svc_dg_data {
	/* XXX: optbuf should be the first field, used by ti_opts.c code */
	struct	netbuf optbuf;			/* netbuf for options */
	long	opts[MAX_OPT_WORDS];		/* options */
	u_int   su_iosz;			/* size of send.recv buffer */
	u_long	su_xid;				/* transaction id */
	XDR	su_xdrs;			/* XDR handle */
	char	su_verfbody[MAX_AUTH_BYTES];	/* verifier body */
	char 	*su_cache;			/* cached data, NULL if none */
};
#define	su_data(xprt)	((struct svc_dg_data *)(xprt->xp_p2))
#define	rpc_buffer(xprt) ((xprt)->xp_p1)

/*
 * Usage:
 *	xprt = svc_dg_create(sock, sendsize, recvsize);
 * Does other connectionless specific initializations.
 * Once *xprt is initialized, it is registered.
 * see (svc.h, xprt_register). If recvsize or sendsize are 0 suitable
 * system defaults are chosen.
 * The routines returns NULL if a problem occurred.
 */

SVCXPRT *
svc_dg_create(fd, sendsize, recvsize)
	register int fd;
	u_int sendsize;
	u_int recvsize;
{
	register SVCXPRT *xprt;
	register struct svc_dg_data *su = NULL;
	struct t_info tinfo;

	trace4(TR_svc_dg_create, 0, fd, sendsize, recvsize);
	if (t_getinfo(fd, &tinfo) == -1) {
		syslog(LOG_ERR, 
		    gettxt("uxnsl:92",
			"svc_dg_create: could not get transport information"));
		trace2(TR_svc_dg_create, 1, fd);
		return ((SVCXPRT *)NULL);
	}
	/*
	 * Find the receive and the send size
	 */
	sendsize = _rpc_get_t_size((int)sendsize, tinfo.tsdu);
	recvsize = _rpc_get_t_size((int)recvsize, tinfo.tsdu);
	if ((sendsize == 0) || (recvsize == 0)) {
		syslog(LOG_ERR, 
		    gettxt("uxnsl:93",
		    "svc_dg_create: transport does not support data transfer"));
		trace2(TR_svc_dg_create, 1, fd);
		return ((SVCXPRT *)NULL);
	}

	xprt = (SVCXPRT *)mem_alloc(sizeof (SVCXPRT));
	if (xprt == NULL)
		goto freedata;
	memset((char *)xprt, 0, sizeof (SVCXPRT));

	su = (struct svc_dg_data *)mem_alloc(sizeof (*su));
	if (su == NULL)
		goto freedata;
	su->su_iosz = ((MAX(sendsize, recvsize) + 3) / 4) * 4;
	if ((rpc_buffer(xprt) = (char *)mem_alloc(su->su_iosz)) == NULL)
		goto freedata;
	xdrmem_create(&(su->su_xdrs), rpc_buffer(xprt), su->su_iosz,
		XDR_DECODE);
	su->su_cache = NULL;
	xprt->xp_fd = fd;
	xprt->xp_p2 = (caddr_t)su;
	xprt->xp_verf.oa_base = su->su_verfbody;
	xprt->xp_ops = svc_dg_ops();
	xprt_register(xprt);
	trace2(TR_svc_dg_create, 1, fd);
	return (xprt);
freedata:
	(void) syslog(LOG_ERR,
	    gettxt("uxnsl:32", "%s: out of memory"),
	    "svc_dg_create");
	if (xprt) {
		if (su)
			(void) mem_free((char *) su, sizeof (*su));
		(void) mem_free((char *)xprt, sizeof (SVCXPRT));
	}
	trace2(TR_svc_dg_create, 1, fd);
	return ((SVCXPRT *)NULL);
}

static enum xprt_stat
svc_dg_stat(xprt)
	SVCXPRT *xprt;
{
	trace1(TR_svc_dg_stat, 0);
	trace1(TR_svc_dg_stat, 1);
	return (XPRT_IDLE);
}

static bool_t
svc_dg_recv(xprt, msg)
	register SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svc_dg_data *su = su_data(xprt);
	register XDR *xdrs = &(su->su_xdrs);
	struct t_unitdata tu_data;
	int moreflag;

	/* XXX: tudata should have been made a part of the server handle */
	trace1(TR_svc_dg_recv, 0);
	tu_data.addr = xprt->xp_rtaddr;
	tu_data.udata.buf = (char *)rpc_buffer(xprt);
	tu_data.opt.buf = (char *) su->opts;
	tu_data.udata.maxlen = su->su_iosz;
	tu_data.opt.maxlen = MAX_OPT_WORDS << 2;  /* no of bytes */

again:
	tu_data.addr.len = 0;
	tu_data.opt.len  = 0;
	tu_data.udata.len  = 0;

	moreflag = 0;
	if (t_rcvudata(xprt->xp_fd, &tu_data, &moreflag) == -1) {
#ifdef RPC_DEBUG
		syslog(LOG_ERR, "svc_dg_recv: t_rcvudata t_errno=%d errno=%d\n",
				t_errno, errno);
#endif
		if (t_errno == TLOOK) {
			int lookres;

			lookres = t_look(xprt->xp_fd);
			if ((lookres & T_UDERR) &&
				(t_rcvuderr(xprt->xp_fd,
					(struct t_uderr *) 0) < 0)) {
#ifdef RPC_DEBUG
				syslog(LOG_ERR,
				"svc_dg_recv: t_rcvuderr t_errno = %d\n",
					t_errno);
#endif
			}
			if (lookres & T_DATA)
				goto again;
		} else if ((errno == EINTR) && (t_errno == TSYSERR))
			goto again;
		else {
			trace1(TR_svc_dg_recv, 1);
			return (FALSE);
		}
	}

	if ((moreflag) || (tu_data.udata.len < 4 * sizeof (u_long))) {
		/*
		 * If moreflag is set, drop that data packet. Something wrong
		 */
		trace1(TR_svc_dg_recv, 1);
		return (FALSE);
	}
	su->optbuf = tu_data.opt;
	xprt->xp_rtaddr.len = tu_data.addr.len;
	xdrs->x_op = XDR_DECODE;
	XDR_SETPOS(xdrs, 0);
	if (! xdr_callmsg(xdrs, msg)) {
		trace1(TR_svc_dg_recv, 1);
		return (FALSE);
	}
	su->su_xid = msg->rm_xid;
	if (su->su_cache != NULL) {
		char *reply;
		u_long replylen;

		if (cache_get(xprt, msg, &reply, &replylen)) {
			/* tu_data.addr is already set */
			tu_data.udata.buf = reply;
			tu_data.udata.len = (u_int)replylen;
			tu_data.opt.len = 0;
			(void) t_sndudata(xprt->xp_fd, &tu_data);
			trace1(TR_svc_dg_recv, 1);
			return (FALSE);
		}
	}
	trace1(TR_svc_dg_recv, 1);
	return (TRUE);
}

static bool_t
svc_dg_reply(xprt, msg)
	register SVCXPRT *xprt;
	struct rpc_msg *msg;
{
	register struct svc_dg_data *su = su_data(xprt);
	register XDR *xdrs = &(su->su_xdrs);
	register bool_t stat = FALSE;

	trace1(TR_svc_dg_reply, 0);
	xdrs->x_op = XDR_ENCODE;
	XDR_SETPOS(xdrs, 0);
	msg->rm_xid = su->su_xid;
	if (xdr_replymsg(xdrs, msg)) {
		register int slen;
		struct t_unitdata tu_data;

		slen = (int)XDR_GETPOS(xdrs);
		tu_data.addr = xprt->xp_rtaddr;
		tu_data.udata.buf = rpc_buffer(xprt);
		tu_data.udata.len = slen;
		tu_data.opt.len = 0;
		if (t_sndudata(xprt->xp_fd, &tu_data) == 0) {
			stat = TRUE;
			if (su->su_cache && slen >= 0) {
				cache_set(xprt, (u_long) slen);
			}
		} else {
			syslog(LOG_ERR,
			    gettxt("uxnsl:94",
			"svc_dg_reply: t_sndudata error t_errno=%d errno=%d\n"),
			    t_errno, errno);
		}
	}
	trace1(TR_svc_dg_reply, 1);
	return (stat);
}

static bool_t
svc_dg_getargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	bool_t dummy_stat1;

	trace1(TR_svc_dg_getargs, 0);
	dummy_stat1 = (*xdr_args)(&(su_data(xprt)->su_xdrs), args_ptr);
	trace1(TR_svc_dg_getargs, 1);
	return (dummy_stat1);
}

static bool_t
svc_dg_freeargs(xprt, xdr_args, args_ptr)
	SVCXPRT *xprt;
	xdrproc_t xdr_args;
	caddr_t args_ptr;
{
	register XDR *xdrs = &(su_data(xprt)->su_xdrs);
	bool_t dummy_stat2;

	trace1(TR_svc_dg_freeargs, 0);
	xdrs->x_op = XDR_FREE;
	dummy_stat2 =  (*xdr_args)(xdrs, args_ptr);
	trace1(TR_svc_dg_freeargs, 1);
	return (dummy_stat2);
}

static void
svc_dg_destroy(xprt)
	register SVCXPRT *xprt;
{
	register struct svc_dg_data *su = su_data(xprt);

	trace1(TR_svc_dg_destroy, 0);
	xprt_unregister(xprt);
	(void) t_close(xprt->xp_fd);
	XDR_DESTROY(&(su->su_xdrs));
	(void) mem_free(rpc_buffer(xprt), su->su_iosz);
	(void) mem_free((caddr_t)su, sizeof (*su));
	if (xprt->xp_rtaddr.buf)
		(void) mem_free(xprt->xp_rtaddr.buf, xprt->xp_rtaddr.maxlen);
	if (xprt->xp_ltaddr.buf)
		(void) mem_free(xprt->xp_ltaddr.buf, xprt->xp_ltaddr.maxlen);
	if (xprt->xp_tp)
		(void) free(xprt->xp_tp);
	(void) mem_free((caddr_t)xprt, sizeof (SVCXPRT));
	trace1(TR_svc_dg_destroy, 1);
}

static struct xp_ops *
svc_dg_ops()
{
	static struct xp_ops ops;

	trace1(TR_svc_dg_ops, 0);
	if (ops.xp_recv == NULL) {
		ops.xp_recv = svc_dg_recv;
		ops.xp_stat = svc_dg_stat;
		ops.xp_getargs = svc_dg_getargs;
		ops.xp_reply = svc_dg_reply;
		ops.xp_freeargs = svc_dg_freeargs;
		ops.xp_destroy = svc_dg_destroy;
	}
	trace1(TR_svc_dg_ops, 1);
	return (&ops);
}

/*  The CACHING COMPONENT */

/*
 * Could have been a separate file, but some part of it depends upon the
 * private structure of the client handle.
 *
 * Fifo cache for cl server
 * Copies pointers to reply buffers into fifo cache
 * Buffers are sent again if retransmissions are detected.
 */

#define	SPARSENESS 4	/* 75% sparse */

#define	ALLOC(type, size)	\
	(type *) mem_alloc((unsigned) (sizeof (type) * (size)))

#define	MEMZERO(addr, type, size)	 \
	(void) memset((char *) (addr), 0, sizeof (type) * (int) (size))

#define	FREE(addr, type, size)	\
	(type *) mem_free((char *) (addr), (sizeof (type) * (size)))

/*
 * An entry in the cache
 */
typedef struct cache_node *cache_ptr;
struct cache_node {
	/*
	 * Index into cache is xid, proc, vers, prog and address
	 */
	u_long cache_xid;
	u_long cache_proc;
	u_long cache_vers;
	u_long cache_prog;
	struct netbuf cache_addr;
	/*
	 * The cached reply and length
	 */
	char *cache_reply;
	u_long cache_replylen;
	/*
	 * Next node on the list, if there is a collision
	 */
	cache_ptr cache_next;
};

/*
 * The entire cache
 */
struct cl_cache {
	u_long uc_size;		/* size of cache */
	cache_ptr *uc_entries;	/* hash table of entries in cache */
	cache_ptr *uc_fifo;	/* fifo list of entries in cache */
	u_long uc_nextvictim;	/* points to next victim in fifo list */
	u_long uc_prog;		/* saved program number */
	u_long uc_vers;		/* saved version number */
	u_long uc_proc;		/* saved procedure number */
};


/*
 * the hashing function
 */
#define	CACHE_LOC(transp, xid)	\
	(xid % (SPARSENESS * ((struct cl_cache *) \
		su_data(transp)->su_cache)->uc_size))
/*
 * Enable use of the cache. Returns 1 on success, 0 on failure.
 * Note: there is no disable.
 */

int
svc_dg_enablecache(transp, size)
	SVCXPRT *transp;
	u_long size;
{
	struct svc_dg_data *su = su_data(transp);
	struct cl_cache *uc;

	trace2(TR_svc_dg_enablecache, 0, size);
	if (su->su_cache != NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:95",
			"svc_dg_enablecache: cache already enabled"));
		trace2(TR_svc_dg_enablecache, 1, size);
		return (0);
	}
	uc = ALLOC(struct cl_cache, 1);
	if (uc == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:96",
			"svc_dg_enablecache: could not allocate cache "));
		trace2(TR_svc_dg_enablecache, 1, size);
		return (0);
	}
	uc->uc_size = size;
	uc->uc_nextvictim = 0;
	uc->uc_entries = ALLOC(cache_ptr, size * SPARSENESS);
	if (uc->uc_entries == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:97",
			"svc_dg_enablecache: could not allocate cache data"));
		FREE(uc, struct cl_cache, 1);
		trace2(TR_svc_dg_enablecache, 1, size);
		return (0);
	}
	MEMZERO(uc->uc_entries, cache_ptr, size * SPARSENESS);
	uc->uc_fifo = ALLOC(cache_ptr, size);
	if (uc->uc_fifo == NULL) {
		(void) syslog(LOG_ERR, 
		    gettxt("uxnsl:98",
			"svc_dg_enablecache: could not allocate cache fifo"));
		FREE(uc->uc_entries, cache_ptr, size * SPARSENESS);
		FREE(uc, struct cl_cache, 1);
		trace2(TR_svc_dg_enablecache, 1, size);
		return (0);
	}
	MEMZERO(uc->uc_fifo, cache_ptr, size);
	su->su_cache = (char *) uc;
	trace2(TR_svc_dg_enablecache, 1, size);
	return (1);
}

/*
 * Set an entry in the cache.  It assumes that the uc entry is set from
 * the earlier call to cache_get() for the same procedure.  This will always
 * happen because cache_get() is calle by svc_dg_recv and cache_set() is called
 * by svc_dg_reply().  All this hoopla because the right RPC parameters are
 * not available at svc_dg_reply time.
 */

static
cache_set(xprt, replylen)
	SVCXPRT *xprt;
	u_long replylen;
{
	register cache_ptr victim;
	register cache_ptr *vicp;
	register struct svc_dg_data *su = su_data(xprt);
	struct cl_cache *uc = (struct cl_cache *) su->su_cache;
	u_int loc;
	char *newbuf;
#ifdef RPC_CACHE_DEBUG
	struct netconfig *nconf;
	char *uaddr;
#endif

	/*
	 * Find space for the new entry, either by
	 * reusing an old entry, or by mallocing a new one
	 */
	trace2(TR_cache_set, 0, replylen);
	victim = uc->uc_fifo[uc->uc_nextvictim];
	if (victim != NULL) {
		loc = CACHE_LOC(xprt, victim->cache_xid);
		for (vicp = &uc->uc_entries[loc];
			*vicp != NULL && *vicp != victim;
			vicp = &(*vicp)->cache_next)
			;
		if (*vicp == NULL) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:99",
				"cache_set: victim not found"));
			trace2(TR_cache_set, 1, replylen);
			return;
		}
		*vicp = victim->cache_next;	/* remove from cache */
		newbuf = victim->cache_reply;
	} else {
		victim = ALLOC(struct cache_node, 1);
		if (victim == NULL) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:100",
				"cache_set: victim alloc failed"));
			trace2(TR_cache_set, 1, replylen);
			return;
		}
		newbuf = (char *)mem_alloc(su->su_iosz);
		if (newbuf == NULL) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:101",
			       "cache_set: could not allocate new rpc buffer"));
			FREE(victim, struct cache_node, 1);
			trace2(TR_cache_set, 1, replylen);
			return;
		}
	}

	/*
	 * Store it away
	 */
#ifdef RPC_CACHE_DEBUG
	if (nconf = getnetconfigent(xprt->xp_netid)) {
		uaddr = taddr2uaddr(nconf, &xprt->xp_rtaddr);
		freenetconfigent(nconf);
		printf(
	"cache set for xid= %x prog=%d vers=%d proc=%d for rmtaddr=%s\n",
			su->su_xid, uc->uc_prog, uc->uc_vers,
			uc->uc_proc, uaddr);
		free(uaddr);
	}
#endif
	victim->cache_replylen = replylen;
	victim->cache_reply = rpc_buffer(xprt);
	rpc_buffer(xprt) = newbuf;
	xdrmem_create(&(su->su_xdrs), rpc_buffer(xprt),
			su->su_iosz, XDR_ENCODE);
	victim->cache_xid = su->su_xid;
	victim->cache_proc = uc->uc_proc;
	victim->cache_vers = uc->uc_vers;
	victim->cache_prog = uc->uc_prog;
	victim->cache_addr = xprt->xp_rtaddr;
	victim->cache_addr.buf = ALLOC(char, xprt->xp_rtaddr.len);
	(void) memcpy(victim->cache_addr.buf, xprt->xp_rtaddr.buf,
			(int)xprt->xp_rtaddr.len);
	loc = CACHE_LOC(xprt, victim->cache_xid);
	victim->cache_next = uc->uc_entries[loc];
	uc->uc_entries[loc] = victim;
	uc->uc_fifo[uc->uc_nextvictim++] = victim;
	uc->uc_nextvictim %= uc->uc_size;
	trace2(TR_cache_set, 1, replylen);
}

/*
 * Try to get an entry from the cache
 * return 1 if found, 0 if not found and set the stage for cache_set()
 */
static int
cache_get(xprt, msg, replyp, replylenp)
	SVCXPRT *xprt;
	struct rpc_msg *msg;
	char **replyp;
	u_long *replylenp;
{
	u_int loc;
	register cache_ptr ent;
	register struct svc_dg_data *su = su_data(xprt);
	register struct cl_cache *uc = (struct cl_cache *) su->su_cache;
#ifdef RPC_CACHE_DEBUG
	struct netconfig *nconf;
	char *uaddr;
#endif

	trace1(TR_cache_get, 0);
	loc = CACHE_LOC(xprt, su->su_xid);
	for (ent = uc->uc_entries[loc]; ent != NULL; ent = ent->cache_next) {
		if (ent->cache_xid == su->su_xid &&
			ent->cache_proc == msg->rm_call.cb_proc &&
			ent->cache_vers == msg->rm_call.cb_vers &&
			ent->cache_prog == msg->rm_call.cb_prog &&
			ent->cache_addr.len == xprt->xp_rtaddr.len &&
			(memcmp(ent->cache_addr.buf, xprt->xp_rtaddr.buf,
				xprt->xp_rtaddr.len) == 0)) {
#ifdef RPC_CACHE_DEBUG
			if (nconf = getnetconfigent(xprt->xp_netid)) {
				uaddr = taddr2uaddr(nconf, &xprt->xp_rtaddr);
				freenetconfigent(nconf);
				printf(
	"cache entry found for xid=%x prog=%d vers=%d proc=%d for rmtaddr=%s\n",
					su->su_xid, msg->rm_call.cb_prog,
					msg->rm_call.cb_vers,
					msg->rm_call.cb_proc, uaddr);
				free(uaddr);
			}
#endif
			*replyp = ent->cache_reply;
			*replylenp = ent->cache_replylen;
			trace1(TR_cache_get, 1);
			return (1);
		}
	}
	/*
	 * Failed to find entry
	 * Remember a few things so we can do a set later
	 */
	uc->uc_proc = msg->rm_call.cb_proc;
	uc->uc_vers = msg->rm_call.cb_vers;
	uc->uc_prog = msg->rm_call.cb_prog;
	trace1(TR_cache_get, 1);
	return (0);
}

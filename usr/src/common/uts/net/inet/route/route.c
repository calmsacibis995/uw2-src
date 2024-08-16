/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/route/route.c	1.4"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	STREAMware TCP/IP Release 1.0
 *
 *	Copyrighted as an unpublished work.
 *	(c) Copyright 1990 INTERACTIVE Systems Corporation
 *	All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *		PROPRIETARY NOTICE (Combined)
 *
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 *
 *
 *
 *		Copyright Notice
 *
 * Notice of copyright on this source code product does not indicate
 * publication.
 *
 *	(c) 1986,1987,1988.1989,1990  Sun Microsystems, Inc
 *	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *		  All rights reserved.
 *
 */

#include <io/ioctl.h>
#include <io/log/log.h>
#include <io/stream.h>
#include <io/strlog.h>
#include <io/stropts.h>
#include <net/inet/af.h>
#include <net/inet/if.h>
#include <net/inet/in.h>
#include <net/inet/in_kern.h>
#include <net/inet/in_mp.h>
#include <net/inet/in_var.h>
#include <net/inet/ip/ip_hier.h>
#include <net/inet/ip/ip_kern.h>
#include <net/inet/ip/ip_mp.h>
#include <net/inet/ip/ip_str.h>
#include <net/inet/ip/ip_var.h>
#include <net/inet/route/route.h>
#include <net/inet/route/route_kern.h>
#include <net/inet/route/route_mp.h>
#include <net/inet/protosw.h>
#include <net/socket.h>
#include <net/sockio.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>		/* must be last */

lock_t	*netmp_lck;

LKINFO_DECL(netmp_lkinfo, "NETINET:NETMP:netmp_lck", 0);

mblk_t	*rthost[RTHASHSIZ];
mblk_t	*rtnet[RTHASHSIZ];

rwlock_t	*rthost_rwlck[RTHASHSIZ];
rwlock_t	*rtnet_rwlck[RTHASHSIZ];

LKINFO_DECL(rt_lkinfo, "NETINET:ROUTE:rt_lk", 0);

rwlock_t	*prov_rwlck;

LKINFO_DECL(prov_lkinfo, "NETINET:ROUTE:prov_rwlck", 0);

struct in_addr	wildcard;	/* zero valued cookie for wildcard searches */
struct rtstat	rtstat;
int	rthashsize = RTHASHSIZ;	/* for netstat, etc. */

void	rtstart(void);

STATIC int	rtcopyinfo(struct rtentry *, mblk_t **);
STATIC int	rtflush(void);
STATIC int	rtgetsize(void);
STATIC int	rtgettable(mblk_t *);
STATIC void	rtgetstats(mblk_t *);

STATIC int	rt_load(void);
STATIC int	rt_unload(void);

STATIC boolean_t	rtinited;

int	routedevflag = D_NEW|D_MP;

#define DRVNAME "route - Internet routing utilities module"

MOD_MISC_WRAPPER(rt, rt_load, rt_unload, DRVNAME);

/*
 * STATIC int
 * rt_load(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
rt_load(void)
{
#if defined(DEBUG)
	cmn_err(CE_NOTE, "rt_load");
#endif	/* defined(DEBUG) */

	rtstart();

	return 0;
}

/*
 * STATIC int
 * rt_unload(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
STATIC int
rt_unload(void)
{
	int	cnt;

#if defined(DEBUG)
	cmn_err(CE_NOTE, "rt_unload");
#endif	/* defined(DEBUG) */

	ASSERT(netmp_lck != NULL);
	LOCK_DEALLOC(netmp_lck);

	if (rtinited == B_FALSE)	/* nothing else to do? */
		return 0;

	for (cnt = 0; cnt < RTHASHSIZ; ++cnt) {
		ASSERT(rtnet_rwlck[cnt] != NULL);
		RW_DEALLOC(rtnet_rwlck[cnt]);

		ASSERT(rthost_rwlck[cnt] != NULL);
		RW_DEALLOC(rthost_rwlck[cnt]);
	}

	ASSERT(prov_rwlck != NULL);
	RW_DEALLOC(prov_rwlck);

	return 0;
}

/*
 * void
 * rtstart(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
void
rtstart(void)
{
	ASSERT(netmp_lck == NULL);
	if ((netmp_lck = LOCK_ALLOC(NETMP_LCK_HIER, plstr, &netmp_lkinfo,
			KM_NOSLEEP)) == NULL) {
		/*
		 *+ LOCK_ALLOC() failed to acquire space for
		 *+ required networking lock.
		 */
		cmn_err(CE_WARN, "netmp: lock alloc failed");
	}
}

/*
 * int
 * rtstartup(void)
 *
 * Calling/Exit State:
 *	No locks held.
 */
int
rtstartup(void)
{
	pl_t	pl;
	int	cnt;
	int	cleanup_cnt;

	pl = LOCK(netmp_lck, plstr);
	if (rtinited == B_TRUE) {
		UNLOCK(netmp_lck, pl);
		return 1;
	}

	for (cnt = 0; cnt < RTHASHSIZ; ++cnt) {
		if ((rthost_rwlck[cnt] = RW_ALLOC(ROUTE_LCK_HIER, plstr,
				&rt_lkinfo, KM_NOSLEEP)) == NULL)
			break;

		if ((rtnet_rwlck[cnt] = RW_ALLOC(ROUTE_LCK_HIER, plstr,
				&rt_lkinfo, KM_NOSLEEP)) == NULL)
			break;
	}

	if (cnt < RTHASHSIZ) {
		UNLOCK(netmp_lck, pl);
		/*
		 *+ LOCK_ALLOC() failed to allocate space for
		 *+ required locks.	Hasta'
		 */
		cmn_err(CE_WARN,
			"rtstartup: failed to allocate required locks");

		for (cleanup_cnt = cnt - 1; cleanup_cnt >= 0; --cleanup_cnt) {
			RW_DEALLOC(rtnet_rwlck[cleanup_cnt]);
			RW_DEALLOC(rthost_rwlck[cleanup_cnt]);
		}

		return 0;
	}

	prov_rwlck = RW_ALLOC(PROV_LCK_HIER, plstr, &prov_lkinfo, KM_NOSLEEP);
	if (prov_rwlck == NULL) {
		UNLOCK(netmp_lck, pl);
		/*
		 *+ RW_ALLOC() failed to allocate space for required lock.
		 */
		cmn_err(CE_WARN,
			"rtstartup: failed to allocate provider lock.");

		for (cleanup_cnt = cnt - 1; cleanup_cnt >= 0; --cleanup_cnt) {
			RW_DEALLOC(rtnet_rwlck[cleanup_cnt]);
			RW_DEALLOC(rthost_rwlck[cleanup_cnt]);
		}

		return 0;
	}

	rtinited = B_TRUE;

	UNLOCK(netmp_lck, pl);

	return 1;
}

/*
 * void rtalloc(struct route *ro)
 *	Packet routing routines.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  RDLOCK(prov)
 *	  Also caller is responsible for protecting the route
 *	  structure 'ro' which will be updated here.
 */
void
rtalloc(struct route *ro)
{
	struct rtentry *rt;
	mblk_t *bp;
	unsigned long hash;
	struct in_addr	dst;
	int doinghost;
	struct afhash h;
	mblk_t **table;
	pl_t pl;
	rwlock_t *rt_lck;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	STRLOG(IPM_ID, 0, 9, SL_TRACE, "rtalloc to %x",
	       SATOSIN(&ro->ro_dst)->sin_addr.s_addr);

	dst.s_addr = SATOSIN(&ro->ro_dst)->sin_addr.s_addr;
	if (ro->ro_rt) {
		pl = RW_RDLOCK(BPTORTE(ro->ro_rt)->rt_rwlck, plstr);
		if (BPTORTENTRY(ro->ro_rt)->rt_prov &&
				(BPTORTENTRY(ro->ro_rt)->rt_flags & RTF_UP)) {
			RW_UNLOCK(BPTORTE(ro->ro_rt)->rt_rwlck, pl);
			return;
		}
		RW_UNLOCK(BPTORTE(ro->ro_rt)->rt_rwlck, pl);
	}
	rt_inethash(dst, &h);
	hash = h.afh_hosthash;
	table = rthost;
	doinghost = 1;
	rt_lck	= rthost_rwlck[RTHASHMOD(hash)];
	pl = RW_WRLOCK(rt_lck, plstr);

again:
	for (bp = table[RTHASHMOD(hash)]; bp; bp = bp->b_cont) {
		rt = BPTORTENTRY(bp);
		if (rt->rt_hash != hash)
			continue;
		if (rt->rt_prov == 0) {
			/*
			 *+ No provider has been allocated to this
			 *+ route.
			 */
			cmn_err(CE_WARN, "rtalloc: null prov");
			continue;
		}
		if ((rt->rt_flags & RTF_UP) == 0 ||
				(rt->rt_prov->if_flags & IFF_UP) == 0)
			continue;
		if (doinghost) {
			if (SATOSIN(&rt->rt_dst)->sin_addr.s_addr != dst.s_addr)
				continue;
		} else {
			if (!INET_NETMATCH(SATOSIN(&rt->rt_dst)->sin_addr, dst))
				continue;
		}
		if (dst.s_addr == wildcard.s_addr)
			RTSTAT_INC(rts_wildcard);
		/* found it */

		ro->ro_rt = bp;
		rt->rt_refcnt++;
		RW_UNLOCK(rt_lck, pl);
		return;
	}
	RW_UNLOCK(rt_lck, pl);

	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash, table = rtnet;
		rt_lck = rtnet_rwlck[RTHASHMOD(hash)];
		pl = RW_WRLOCK(rt_lck, plstr);
		goto again;
	}
	/*
	 * Check for wildcard gateway, by convention network 0.
	 */
	if (dst.s_addr != wildcard.s_addr) {
		dst.s_addr = wildcard.s_addr, hash = 0;
		rt_lck = rtnet_rwlck[RTHASHMOD(hash)];
		pl = RW_WRLOCK(rt_lck, plstr);
		goto again;
	}
	RTSTAT_INC(rts_unreach);
	return;
}

/*
 * void rtfree(mblk_t *bp, int flag)
 *	free route entry.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  IFF flag==MP_NOLOCK then WRLOCK(rtlist) must be held
 *	  on entry.
 */
void
rtfree(mblk_t *bp, int flag)
{
	struct rte *rte = BPTORTE(bp);
	struct rtentry *rt = &rte->rt_entry;
	pl_t pl;
	rwlock_t *lck;

	ASSERT(rt != 0);
	/* ASSERT((flag == MP_LOCK) || RW_OWNED(rte->rt_rwlck)); */

	lck = rte->rt_rwlck;
	if (flag == MP_LOCK)
		pl = RW_WRLOCK(lck, plstr);

	rt->rt_refcnt--;
	if (rt->rt_refcnt == 0) {
		if ((rt->rt_flags & RTF_UP) == 0) {
			/* this route already removed from hash list */
			freeb(bp);
		}
	}
	if (flag == MP_LOCK)
		RW_UNLOCK(lck, pl);
}

/*
 * void rt_inethash(struct in_addr in, struct afhash *hp)
 *	Create a hash table entry for the network part and
 *	host part of address `in'.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  in		Address to hash.
 *	  hp		(returned) ->afh_nethash: hashed network portion.
 *			(returned) ->afh_hosthash: hashed host portion.
 *
 *	Locking:
 *	  RDLOCK(prov)
 */
void
rt_inethash(struct in_addr in, struct afhash *hp)
{
	unsigned long n;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	n = in_netof(in);
	if (n)
		while ((n & 0xff) == 0)
			n >>= 8;
	hp->afh_nethash = n;
	hp->afh_hosthash = ntohl(in.s_addr);
}

/*
 * void rtredirect(struct in_addr dst, struct in_addr gateway, int flags,
 *		   struct in_addr src, int metric, int proto, time_t age)
 *	Force a routing table entry to the specified destination to go
 *	through the given gateway. Normally called as a result of a
 *	routing redirect message from the network layer.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  RDLOCK(prov)
 */
void
rtredirect(struct in_addr dst, struct in_addr gateway, int flags,
	   struct in_addr src, int metric, int proto, time_t age)
{
	struct route ro;
	struct rte *rte;
	struct rtentry *rt;
	mblk_t *bp;
	pl_t pl;
	int locked = 0;
	rwlock_t *lck;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	/* verify the gateway is directly reachable */
	if (prov_withnet(gateway) == 0) {
		RTSTAT_INC(rts_badredirect);
		return;
	}
	SATOSIN(&ro.ro_dst)->sin_addr.s_addr = dst.s_addr;
	ro.ro_rt = 0;
	rtalloc(&ro);
	bp = ro.ro_rt;

	if (bp) {
		rte = BPTORTE(bp);
		rt = &rte->rt_entry;
		lck = rte->rt_rwlck;
		pl = RW_WRLOCK(lck, plstr);
		locked = 1;
	}

	/*
	 * If the redirect isn't from our current router for this dst, it's
	 * either old or wrong.	 If it redirects us to ourselves, we have a
	 * routing loop, perhaps as a result of an interface going down
	 * recently.
	 */
	if ((bp && src.s_addr != SATOSIN(&rt->rt_gateway)->sin_addr.s_addr) ||
	    prov_withaddr(gateway)) {
		RTSTAT_INC(rts_badredirect);
		if (bp) {
			rtfree(bp, MP_NOLOCK);
			RW_UNLOCK(lck, pl);
		}
		return;
	}
	/*
	 * Create a new entry if we just got back a wildcard entry or the
	 * lookup failed.  This is necessary for hosts which use routing
	 * redirects generated by smart gateways to dynamically build the
	 * routing tables.
	 */
	if (bp && INET_NETMATCH(wildcard, SATOSIN(&rt->rt_dst)->sin_addr)) {
		rtfree(bp, MP_NOLOCK);
		RW_UNLOCK(lck, pl);
		locked = 0;
		bp = 0;
	}
	if (!bp) {
		rtinit(dst, gateway, SIOCADDRT,
		       (flags & RTF_HOST) | RTF_GATEWAY | RTF_DYNAMIC, metric,
		       proto, age);
		RTSTAT_INC(rts_dynamic);
		if (locked)
			RW_UNLOCK(lck, pl);
		return;
	}
	/*
	 * Don't listen to the redirect if it's for a route to an interface.
	 */
	if (rt->rt_flags & RTF_GATEWAY) {
		if (!(rt->rt_flags & RTF_HOST) && (flags & RTF_HOST)) {
			/*
			 * Changing from route to net => route to host.
			 * Create new route, rather than smashing route to
			 * net.
			 */
			rt->rt_flags |= RTF_MODIFIED;
			RW_UNLOCK(rte->rt_rwlck, pl);
			locked = 0;
			rtinit(dst, gateway, SIOCADDRT, flags | RTF_DYNAMIC,
			       metric, proto, age);
			RTSTAT_INC(rts_dynamic);
		} else {
			/*
			 * Smash the current notion of the gateway to this
			 * destination.
			 */
			SATOSIN(&rt->rt_gateway)->sin_addr = gateway;
			rt->rt_flags |= RTF_MODIFIED;
			RW_UNLOCK(rte->rt_rwlck, pl);
			locked = 0;
		}
		RTSTAT_INC(rts_newgateway);
	} else
		RTSTAT_INC(rts_badredirect);

	rtfree(bp, locked ? MP_NOLOCK : MP_LOCK);

	if (locked)
		RW_UNLOCK(lck, pl);
}

/*
 * void rtioctl(int cmd, mblk_t *bp)
 *	Routing table ioctl interface.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  Called by ipioctl().
 *	  No locks held on entry.
 */
void
rtioctl(int cmd, mblk_t *bp)
{
	struct iocblk *iocbp = BPTOIOCBLK(bp);
	pl_t pl;

	iocbp->ioc_error = 0;
	iocbp->ioc_rval = 0;

	switch ((unsigned int)cmd) {
	case SIOCADDRT:
	case SIOCDELRT:
		if (bp->b_cont == NULL)
			iocbp->ioc_error = EINVAL;
		else {
			pl = RW_RDLOCK(prov_rwlck, plstr);
			iocbp->ioc_error = rtrequest(cmd, bp->b_cont, MP_LOCK);
			RW_UNLOCK(prov_rwlck, pl);
		}
		break;

	case SIOCFLUSHRT:
		iocbp->ioc_error = rtflush();
		break;

	case SIOCGRTSIZ:
		iocbp->ioc_rval = rtgetsize();
		break;

	case SIOCGRTTAB:
		iocbp->ioc_rval = rtgettable(bp->b_cont);
		iocbp->ioc_count = iocbp->ioc_rval * sizeof (struct rtrecord);
		break;

	case SIOCGRTSTATS:
		if ((bp->b_cont == NULL) ||
		    (bp->b_cont->b_datap->db_lim -
		     bp->b_cont->b_datap->db_base) < sizeof (struct rtstat)) {
			iocbp->ioc_rval = -1;
			iocbp->ioc_error = ENOSR;
		}
		else {
			rtgetstats(bp->b_cont);
			iocbp->ioc_count = sizeof (struct rtstat);
		}
		break;

	default:
		iocbp->ioc_error =  EINVAL;
		break;
	}

	bp->b_datap->db_type = iocbp->ioc_error ? M_IOCNAK : M_IOCACK;
}

/*
 * int rtrequest(int req, mblk_t *bp, int flag)
 *	Carry out a request to change the routing table.  Called by
 *	interfaces at boot time to make their ``local routes'' known,
 *	for ioctl's, and as the result of routing redirects.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  RDLOCK(prov)
 *	  if flag==MP_NOLOCK then WRLOCK(rtlist) must be held on entry.
 */
int
rtrequest(int req, mblk_t *bp, int flag)
{
	struct rte *rte = BPTORTE(bp);
	struct rtentry *entry = &rte->rt_entry;
	mblk_t *m, **mprev;
	mblk_t **mfirst;
	struct rtentry *rt;
	struct afhash h;
	int error = 0;
	unsigned long hash;
	struct ip_provider *prov = NULL;
	pl_t pl;
	rwlock_t *lck = 0;

	/* ASSERT(RW_OWNED(prov_rwlck)); */
	/* ASSERT((flag == MP_LOCK) || RW_OWNED(rte->rt_rwlck)); */

	rt_inethash(SATOSIN(&entry->rt_dst)->sin_addr, &h);
	if (entry->rt_flags & RTF_HOST) {
		hash = h.afh_hosthash;
		mprev = &rthost[RTHASHMOD(hash)];
		lck = rthost_rwlck[RTHASHMOD(hash)];
	} else {
		hash = h.afh_nethash;
		mprev = &rtnet[RTHASHMOD(hash)];
		lck = rtnet_rwlck[RTHASHMOD(hash)];
	}

	if (flag == MP_LOCK)
		pl = RW_WRLOCK(lck, plstr);
	for (mfirst = mprev; (m = *mprev) != NULL; mprev = &m->b_cont) {
		rt = BPTORTENTRY(m);
		if (rt->rt_hash != hash)
			continue;
		if (entry->rt_flags & RTF_HOST) {
			if (SATOSIN(&rt->rt_dst)->sin_addr.s_addr !=
				SATOSIN(&entry->rt_dst)->sin_addr.s_addr)
				continue;
		} else {
			if (INET_NETMATCH(SATOSIN(&rt->rt_dst)->sin_addr,
			SATOSIN(&entry->rt_dst)->sin_addr) == 0)
				continue;
		}
		if (SATOSIN(&rt->rt_gateway)->sin_addr.s_addr ==
			SATOSIN(&entry->rt_gateway)->sin_addr.s_addr)
			break;
	}
	switch ((unsigned int)req) {

	case SIOCDELRT:
		if (m == 0) {
			error = ESRCH;
			goto bad;
		}

		*mprev = m->b_cont;
		if (rt->rt_refcnt > 0) {
			rt->rt_flags &= ~RTF_UP;
			m->b_cont = 0;
		} else
			freeb(m);
		break;

	case SIOCADDRT:
		if (m) {
			error = EEXIST;
			goto bad;
		}

		/*
		 * If we are adding a route to an interface, and the
		 * interface is a pt to pt link we should search for the
		 * destination as our clue to the interface.  Otherwise we
		 * can use the local address (below).
		 */
		if (prov == NULL && (entry->rt_flags & RTF_GATEWAY) == 0 &&
		    (entry->rt_flags & RTF_HOST))
			prov = prov_withdstaddr(SATOSIN(&entry->rt_dst)->
						sin_addr);
		if (prov == NULL && !(entry->rt_flags & RTF_GATEWAY))
			prov = prov_withaddr(SATOSIN(&entry->rt_gateway)->
					     sin_addr);
		if (prov == NULL)
			prov = prov_withnet(SATOSIN(&entry->rt_gateway)->
					    sin_addr);
		if (prov == NULL) {
			error = ENETUNREACH;
			goto bad;
		}
		m = allocb(sizeof (struct rte), BPRI_MED);
		if (!m) {
			error = ENOSR;
			goto bad;
		}
		m->b_cont = *mfirst;
		*mfirst = m;
		m->b_wptr += sizeof (struct rte);
		rt = BPTORTENTRY(m);
		rt->rt_hash = hash;
		rt->rt_dst = entry->rt_dst;
		rt->rt_gateway = entry->rt_gateway;
		rt->rt_flags = RTF_UP |
			(entry->rt_flags &
			 (RTF_USERMASK | RTF_TOSWITCH | RTF_DYNAMIC));
		rt->rt_refcnt = 0;
		rt->rt_use = 0;
		rt->rt_prov = prov;
		rt->rt_metric = entry->rt_metric;
		rt->rt_proto = entry->rt_proto;
		rt->rt_age = entry->rt_age;
		BPTORTE(m)->rt_rwlck = lck;
		break;

	default:
		error = EINVAL;
		break;
	}
bad:
	if (flag == MP_LOCK)
		RW_UNLOCK(lck, pl);

	return error;
}

/*
 * void rtinit(struct in_addr dst, struct in_addr gateway, int cmd,
 *	       int flags, int metric, int proto, time_t age)
 *	Set up a routing table entry, normally for an interface.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  RDLOCK(prov)
 */
void
rtinit(struct in_addr dst, struct in_addr gateway, int cmd, int flags,
       int metric, int proto, time_t age)
{
	mblk_t *bp;
	struct rtentry *route;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	bp = allocb(sizeof (struct rte), BPRI_HI);
	if (!bp)
		return;

	bp->b_wptr += sizeof (struct rte);
	route = BPTORTENTRY(bp);

	bzero(route, sizeof *route);

	SATOSIN(&route->rt_dst)->sin_addr = dst;
	SATOSIN(&route->rt_dst)->sin_family = AF_INET;
	SATOSIN(&route->rt_gateway)->sin_addr = gateway;
	SATOSIN(&route->rt_gateway)->sin_family = AF_INET;

	route->rt_flags = (short)flags;
	route->rt_metric = metric;
	route->rt_proto = proto;
	route->rt_age = age;

	(void) rtrequest(cmd, bp, MP_LOCK);
	freeb(bp);
}

/*
 * void rtdetach(struct ip_provider *prov)
 *	Flush all routing table entries for an interface being detached
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  RDLOCK(prov)
 */
void
rtdetach(struct ip_provider *prov)
{
	mblk_t *m, **mprev;
	mblk_t **table;
	unsigned short i, j;
	struct rtentry	*rt;
	pl_t pl;

	/* ASSERT(RW_OWNED(prov_rwlck)); */

	for (j = 0, table = rthost; j < 2; j++, table = rtnet) {
		for (i = 0; i < RTHASHSIZ; i++) {
			if (table[i] == NULL)
				continue;

			pl = RW_WRLOCK((j==0 ? 
				rthost_rwlck[i] : rtnet_rwlck[i]), plstr);

			for (mprev = &table[i]; (m = *mprev) != NULL;
			     mprev = &m->b_cont) {
				rt = BPTORTENTRY(m);
				if (rt->rt_prov == prov) {
					*mprev = m->b_cont;
					if (rt->rt_refcnt > 0) {
						rt->rt_flags &= ~RTF_UP;
						m->b_cont = 0;
					} else
						freeb(m);
				}
			}

			RW_UNLOCK((j==0 ? rthost_rwlck[i] : rtnet_rwlck[i]),
				pl);
		}
	}
}

/*
 * void rtcopy(struct route *from_route, struct route *to_route)
 *
 * Calling/Exit State:
 *	Called by udp_output().
 *	No locks held on entry.
 */
void
rtcopy(struct route *from_route, struct route *to_route)
{
	pl_t pl;
	struct rte *rte;
	struct rtentry *rt;

	bcopy(from_route, to_route, sizeof (struct route));

	if (from_route->ro_rt) {
		rte = BPTORTE(from_route->ro_rt);
		rt = &rte->rt_entry;
		pl = RW_WRLOCK(rte->rt_rwlck, plstr);
		rt->rt_refcnt++;
		RW_UNLOCK(rte->rt_rwlck, pl);
	}
}

/*
 * STATIC int rtflush(void)
 *	Flush all routing table entries.
 *
 * Calling/Exit State:
 *	Locking:
 *	  All rtlists unlocked
 */
STATIC int
rtflush(void)
{
	mblk_t *m, *m2;
	mblk_t **table;
	unsigned short i, j;
	struct rtentry *rt;
	pl_t pl;

	for (j = 0, table = rthost; j < 2; j++, table = rtnet) {
		for (i = 0; i < RTHASHSIZ; i++) {
			if (table[i] == NULL)
				continue;

			/* do not call rtrequest to avoid recursive locking */

			pl = RW_WRLOCK((j==0 ? 
				rthost_rwlck[i] : rtnet_rwlck[i]), plstr);

			m = table[i];
			table[i] = 0;
			while (m) {
				rt = BPTORTENTRY(m);
				m2 = m->b_cont;
				/* delete only gateway routes */
				if (rt->rt_flags & RTF_GATEWAY) {
					if (rt->rt_refcnt > 0) {
						rt->rt_flags &= ~RTF_UP;
						m->b_cont = 0;
					} else {
						(void) freeb(m);
					}
				}
				else {
					/* reconstruct undeleted list */
					m->b_cont = table[i];
					table[i] = m;
				}
				m = m2;
			}

			RW_UNLOCK((j==0 ? rthost_rwlck[i] : rtnet_rwlck[i]), 
				pl);
		}
	}

	return 0;
}

/*
 * STATIC int rtgetsize(void)
 *	Get the total number of rtentry in tables.
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  All rtlists unlocked.
 */
STATIC int
rtgetsize(void)
{
	mblk_t *m;
	mblk_t **table;
	unsigned short i, j, rtcount;
	pl_t pl;

	/* get the total number of rtentry in tables */

	rtcount = 0;
	for (j = 0, table = rthost; j < 2; j++, table = rtnet) {
		for (i = 0; i < RTHASHSIZ; i++) {
			if (table[i] == NULL)
				continue;

			pl = RW_WRLOCK((j==0 ? 
				rthost_rwlck[i] : rtnet_rwlck[i]), plstr);

			for (m=table[i] ; m ; m=m->b_cont) {
				rtcount++;
			}

			RW_UNLOCK((j==0 ? rthost_rwlck[i] : rtnet_rwlck[i]), 
				pl);
		}
	}

	return rtcount;
}

/*
 * STATIC int rtgettable(mblk_t *bp)
 *
 * Calling/Exit State:
 *
 *	Locking:
 *	  All rtlists unlocked
 *	  prov unlocked
 */
STATIC int
rtgettable(mblk_t *bp)
{
	mblk_t *m;
	mblk_t **table;
	int i;
	int rtcount = 0;
	int nospace = 0;
	pl_t pl;

	/* ASSERT(!RW_OWNED(prov_rwlck)); */

	bp->b_rptr = bp->b_wptr = bp->b_datap->db_base;
	/*
	 * prov_rwlck is needed for accessing rt_prov->name in rtcopyinfo. 
	 * We acquire the lock early because of locking hierarchies.
	 */
	pl = RW_RDLOCK(prov_rwlck, plstr);
	table = rthost;
	for (i = 0; i < RTHASHSIZ; i++) {
		if (table[i] == NULL)
			continue;

		(void)RW_RDLOCK(rthost_rwlck[i], plstr);

		for (m = table[i] ; m ; m = m->b_cont) {
			if (nospace = rtcopyinfo(BPTORTENTRY(m), &bp))
				break;
			rtcount++;
		}

		RW_UNLOCK(rthost_rwlck[i], plstr);
		if (nospace) {
			RW_UNLOCK(prov_rwlck, pl);
			return rtcount;
		}
	}
	table = rtnet;
	for (i = 0; i < RTHASHSIZ; i++) {
		if (table[i] == NULL)
			continue;

		(void)RW_RDLOCK(rtnet_rwlck[i], plstr);

		for (m = table[i] ; m ; m = m->b_cont) {
			if (nospace = rtcopyinfo(BPTORTENTRY(m), &bp))
				break;
			rtcount++;
		}

		RW_UNLOCK(rtnet_rwlck[i], plstr);
		if (nospace)
			break;
	}
	RW_UNLOCK(prov_rwlck, pl);
	return rtcount;
}

/*
 * STACTIC int rtcopyinfo(struct rtentry *rt, mblk_t **obp)
 *
 * Calling/Exit State:
 *	prov_rwlock is held in share mode
 *
 *	Notes:
 *		This procedure should probably be inlined.  it
 *		is currently only used once and the (void *) depends
 *		on the calling code to ensure that b_wptr is aligned
 *		correctly for a struct rtrecord.
 */
STATIC int
rtcopyinfo(struct rtentry *rt, mblk_t **obp)
{
	mblk_t *bp = *obp;
	struct rtrecord *rec;

check:
	if ((bp->b_datap->db_lim - bp->b_wptr) < sizeof (struct rtrecord)) {
		if ((bp = bp->b_cont) == NULL) {
			return 1;	/* no space */
		} else {
			bp->b_rptr = bp->b_wptr = bp->b_datap->db_base;
			goto check;
		}
	}
	rec = (struct rtrecord *)(void *)bp->b_wptr;
	rec->rt_dst = rt->rt_dst;
	rec->rt_gateway = rt->rt_gateway;
	rec->rt_flags = rt->rt_flags;
	rec->rt_refcnt = rt->rt_refcnt;
	rec->rt_use = rt->rt_use;
	if (rt->rt_prov) {
		bcopy(rt->rt_prov->name, rec->rt_prov, IFNAMSIZ);
	}
	bp->b_wptr += sizeof (struct rtrecord);
	*obp = bp;

	return 0;
}

/*
 * STATIC void rtgetstats(mblk_t *bp)
 *	Copy routing statistics to be sent to user land.
 *
 * Calling/Exit State:
 *	Parameters:
 *	  bp		Where to copy stats.
 *
 *	Locking:
 *	  None.
 */
STATIC void
rtgetstats(mblk_t *bp)
{
	struct rtstat *copy;

	bp->b_rptr = bp->b_datap->db_base;
	bp->b_wptr = bp->b_rptr + sizeof (struct rtstat);
	copy = BPTORTSTAT(bp);
	/*
	 * Do a structure copy
	 * NO STAT LOCKS are used
	 */
	*copy = rtstat;
}

#if defined(DEBUG)
/*
 * void print_route_lkstats(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
print_route_lkstats(void)
{
	lkstat_t	*lsp;
	int	cnt;

	debug_printf("Dumping route locking statistics:\n\n");

	debug_printf("rthost_rwlck\tWrite\tRead\tFail\tSolo Read\n");
	debug_printf("entry\t\tcnt\tcnt\tcnt\tcnt\n");

	for (cnt = 0; cnt < RTHASHSIZ; cnt++) {
		if ((lsp = rthost_rwlck[cnt]->rws_lkstatp) != NULL) {
			debug_printf("%d\t\t%d\t%d\t%d\t%d\n", cnt,
				lsp->lks_wrcnt, lsp->lks_rdcnt,
				lsp->lks_fail, lsp->lks_solordcnt);
		}
	}
	debug_printf("\n");
	debug_printf("rtnet_rwlck\tWrite\tRead\tFail\tSolo Read\n");
	debug_printf("entry\t\tcnt\tcnt\tcnt\tcnt\n");
	for (cnt = 0; cnt < RTHASHSIZ; cnt++) {
		if ((lsp = rtnet_rwlck[cnt]->rws_lkstatp) != NULL) {
			debug_printf("%d\t\t%d\t%d\t%d\t%d\n", cnt,
				lsp->lks_wrcnt, lsp->lks_rdcnt,
				lsp->lks_fail, lsp->lks_solordcnt);
		}
	}

	debug_printf("\n");
}
#endif /* defined(DEBUG) */

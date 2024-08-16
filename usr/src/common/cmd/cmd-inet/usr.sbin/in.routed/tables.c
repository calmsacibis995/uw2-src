/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.routed/tables.c	1.2.8.3"
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
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
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
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1983, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)tables.c	5.15 (Berkeley) 2/18/89
 */

/*
 * Routing Table Management Daemon
 */
#include "defs.h"
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/syslog.h>

#ifndef DEBUG
#define	DEBUG	0
#endif

int	install = !DEBUG;		/* if 1 call kernel */

/*
 * Lookup dst in the tables for an exact match.
 */
struct rt_entry *
rtlookup(dst)
	struct sockaddr *dst;
{
	register struct rt_entry *rt;
	register struct rthash *rh;
	register u_int hash;
	struct afhash h;
	int doinghost = 1;

	if ((int) dst->sa_family >= af_max)
		return (0);
	(*afswitch[dst->sa_family].af_hash)(dst, &h);
	hash = h.afh_hosthash;
	rh = &hosthash[hash & ROUTEHASHMASK];
again:
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		if (rt->rt_Hash != hash)
			continue;
		if (equal(&rt->rt_Dst, dst))
			return (rt);
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
		goto again;
	}
	return (0);
}

struct sockaddr wildcard;	/* zero valued cookie for wildcard searches */

/*
 * Find a route to dst as the kernel would.
 */
struct rt_entry *
rtfind(dst)
	struct sockaddr *dst;
{
	register struct rt_entry *rt;
	register struct rthash *rh;
	register u_int hash;
	struct afhash h;
	int af = dst->sa_family;
	int doinghost = 1, (*match)();

	if (af >= af_max)
		return (0);
	(*afswitch[af].af_hash)(dst, &h);
	hash = h.afh_hosthash;
	rh = &hosthash[hash & ROUTEHASHMASK];

again:
	for (rt = rh->rt_forw; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
		if (rt->rt_Hash != hash)
			continue;
		if (doinghost) {
			if (equal(&rt->rt_Dst, dst))
				return (rt);
		} else {
			if (rt->rt_Dst.sa_family == af &&
			    (*match)(&rt->rt_Dst, dst))
				return (rt);
		}
	}
	if (doinghost) {
		doinghost = 0;
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
		match = afswitch[af].af_netmatch;
		goto again;
	}
#ifdef notyet
	/*
	 * Check for wildcard gateway, by convention network 0.
	 */
	if (dst != &wildcard) {
		dst = &wildcard, hash = 0;
		goto again;
	}
#endif
	return (0);
}

rtadd(dst, gate, metric, state, proto)
	struct sockaddr *dst, *gate;
	int metric, state, proto;
{
	struct afhash h;
	register struct rt_entry *rt;
	struct rthash *rh;
	int af = dst->sa_family, flags;
	u_int hash;

	if (af >= af_max)
		return;
	(*afswitch[af].af_hash)(dst, &h);
	flags = (*afswitch[af].af_rtflags)(dst);
	/*
	 * Subnet flag isn't visible to kernel, move to state.	XXX
	 */
	if (flags & RTF_SUBNET) {
		state |= RTS_SUBNET;
		flags &= ~RTF_SUBNET;
	}
	if (flags & RTF_HOST) {
		hash = h.afh_hosthash;
		rh = &hosthash[hash & ROUTEHASHMASK];
	} else {
		hash = h.afh_nethash;
		rh = &nethash[hash & ROUTEHASHMASK];
	}
	rt = (struct rt_entry *)malloc(sizeof (*rt));
	if (rt == 0)
		return;
	rt->rt_Hash = hash;
	rt->rt_Dst = *dst;
	rt->rt_Router = *gate;
	rt->rt_timer = 0;
	rt->rt_Flags = RTF_UP | flags;
	rt->rt_State = state | RTS_CHANGED;
	rt->rt_ifp = if_ifwithdstaddr(&rt->rt_Router);
	if (rt->rt_ifp == 0)
		rt->rt_ifp = if_ifwithnet(&rt->rt_Router);
	if ((state & RTS_INTERFACE) == 0)
		rt->rt_Flags |= RTF_GATEWAY;
	rt->rt_Metric = metric;
	rt->rt_Proto = proto;
	rt->rt_Age = time((long *)0);
	insque(rt, rh);
	TRACE_ACTION("ADD", rt);
	/*
	 * If the ioctl fails because the gateway is unreachable
	 * from this host, discard the entry.  This should only
	 * occur because of an incorrect entry in /etc/gateways.
	 */
	if (install && (rt->rt_State & (RTS_INTERNAL | RTS_EXTERNAL)) == 0 &&
	    ioctl(s, SIOCADDRT, (char *)&rt->rt_rt) < 0) {
		if (errno != EEXIST && (int) gate->sa_family < af_max)
			syslog(LOG_ERR,
			"adding route to net/host %s through gateway %s: %m\n",
			   (*afswitch[dst->sa_family].af_format)(dst),
			   (*afswitch[gate->sa_family].af_format)(gate));
		if (errno != EEXIST)
			perror("SIOCADDRT");
		if (errno == ENETUNREACH) {
			TRACE_ACTION("DELETE", rt);
			remque(rt);
			free((char *)rt);
		}
	}
}

rtchange(rt, gate, metric, proto)
	register struct rt_entry *rt;
	struct sockaddr *gate;
	short metric; 
	int proto;
{
	int add = 0, delete = 0, newgateway = 0;
	struct rtentry oldroute;

	if (!equal(&rt->rt_Router, gate)) {
		newgateway++;
		TRACE_ACTION("CHANGE FROM ", rt);
	} else if (metric != rt->rt_Metric)
		TRACE_NEWMETRIC(rt, metric);
	if ((rt->rt_State & RTS_INTERNAL) == 0) {
		/*
		 * If changing to different router, we need to add
		 * new route and delete old one if in the kernel.
		 * If the router is the same, we need to delete
		 * the route if has become unreachable, or re-add
		 * it if it had been unreachable.
		 */
		if (newgateway) {
			add++;
			if (rt->rt_Metric != HOPCNT_INFINITY)
				delete++;
		} else if (metric == HOPCNT_INFINITY)
			delete++;
		else if (rt->rt_Metric == HOPCNT_INFINITY)
			add++;
	}
	if (delete)
		oldroute = rt->rt_rt;
	if ((rt->rt_State & RTS_INTERFACE) && delete) {
		rt->rt_State &= ~RTS_INTERFACE;
		rt->rt_Flags |= RTF_GATEWAY;
		if (metric > rt->rt_Metric && delete)
			syslog(LOG_ERR, "%s route to interface %s (timed out)",
			    add? "changing" : "deleting",
			    rt->rt_ifp->int_name);
	}
	if (add) {
		rt->rt_Router = *gate;
		rt->rt_ifp = if_ifwithdstaddr(&rt->rt_Router);
		if (rt->rt_ifp == 0)
			rt->rt_ifp = if_ifwithnet(&rt->rt_Router);
	}
	rt->rt_Metric = metric;
	rt->rt_Proto = proto;
	rt->rt_Age = time((long *)0);
	rt->rt_State |= RTS_CHANGED;
	if (newgateway)
		TRACE_ACTION("CHANGE TO   ", rt);
	if (add && install)
		if ((ioctl(s, SIOCADDRT, (char *)&rt->rt_rt) < 0) &&
		    !(add && delete && install))
			perror("SIOCADDRT");
	if (delete && install)
		if (ioctl(s, SIOCDELRT, (char *)&oldroute) < 0)
			perror("SIOCDELRT");
	/* in case first one failed because only metric changed */
	if (add && delete && install)
		ioctl(s, SIOCADDRT, (char *)&rt->rt_rt);
}

rtdelete(rt)
	struct rt_entry *rt;
{

	TRACE_ACTION("DELETE", rt);
	if (rt->rt_Metric < HOPCNT_INFINITY) {
	    if ((rt->rt_State & (RTS_INTERFACE|RTS_INTERNAL)) == RTS_INTERFACE)
		syslog(LOG_ERR,
		    "deleting route to interface %s? (timed out?)",
		    rt->rt_ifp->int_name);
	    if (install &&
		(rt->rt_State & (RTS_INTERNAL | RTS_EXTERNAL)) == 0 &&
		ioctl(s, SIOCDELRT, (char *)&rt->rt_rt))
		    perror("SIOCDELRT");
	}
	remque(rt);
	free((char *)rt);
}

void
rtdeleteall(sig)
	int sig;
{
	register struct rthash *rh;
	register struct rt_entry *rt;
	struct rthash *base = hosthash;
	int doinghost = 1;

again:
	for (rh = base; rh < &base[ROUTEHASHSIZ]; rh++) {
		rt = rh->rt_forw;
		for (; rt != (struct rt_entry *)rh; rt = rt->rt_forw) {
			if (rt->rt_State & RTS_INTERFACE ||
			    rt->rt_Metric >= HOPCNT_INFINITY)
				continue;
			TRACE_ACTION("DELETE", rt);
			if ((rt->rt_State & (RTS_INTERNAL|RTS_EXTERNAL)) == 0 &&
			    ioctl(s, SIOCDELRT, (char *)&rt->rt_rt))
				perror("SIOCDELRT");
		}
	}
	if (doinghost) {
		doinghost = 0;
		base = nethash;
		goto again;
	}
	exit(sig);
}

/*
 * If we have an interface to the wide, wide world,
 * add an entry for an Internet default route (wildcard) to the internal
 * tables and advertise it.  This route is not added to the kernel routes,
 * but this entry prevents us from listening to other people's defaults
 * and installing them in the kernel here.
 */
rtdefault()
{
	extern struct sockaddr inet_default;

	rtadd(&inet_default, &inet_default, 1,
		RTS_CHANGED | RTS_PASSIVE | RTS_INTERNAL, RTP_LOCAL);
}

rtinit()
{
	register struct rthash *rh;

	for (rh = nethash; rh < &nethash[ROUTEHASHSIZ]; rh++)
		rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
	for (rh = hosthash; rh < &hosthash[ROUTEHASHSIZ]; rh++)
		rh->rt_forw = rh->rt_back = (struct rt_entry *)rh;
}

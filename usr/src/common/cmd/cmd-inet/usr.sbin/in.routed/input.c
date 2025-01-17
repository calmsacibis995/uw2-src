/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.routed/input.c	1.1.8.2"
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
 *	@(#)input.c	5.20 (Berkeley) 2/20/89
 */

/*
 * Routing Table Management Daemon
 */
#include "defs.h"
#include <sys/syslog.h>

#ifndef min
#define min(a,b)	((a) < (b) ? (a) : (b))
#endif

/*
 * Process a newly received packet.
 */
rip_input(from, rip, size)
	struct sockaddr *from;
	register struct rip *rip;
	int size;
{
	register struct rt_entry *rt;
	register struct netinfo *n;
	register struct interface *ifp;
	struct interface *if_ifwithdstaddr();
	int count, changes = 0;
	register struct afswitch *afp;
	static struct sockaddr badfrom, badfrom2;

	ifp = 0;
	TRACE_INPUT(ifp, from, (char *)rip, size);
	if ((int) from->sa_family >= af_max ||
	    (afp = &afswitch[from->sa_family])->af_hash == (int (*)())0) {
		syslog(LOG_INFO,
	 "\"from\" address in unsupported address family (%d), cmd %d\n",
		    from->sa_family, rip->rip_cmd);
		return;
	}
	if (rip->rip_vers == 0) {
		syslog(LOG_ERR,
		    "RIP version 0 packet received from %s! (cmd %d)",
		    (*afswitch[from->sa_family].af_format)(from), rip->rip_cmd);
		return;
	}
	switch (rip->rip_cmd) {

	case RIPCMD_REQUEST:
		n = rip->rip_nets;
		count = size - ((char *)n - (char *)rip);
		if (count < sizeof (struct netinfo))
			return;
		for (; count > 0; n++) {
			if (count < sizeof (struct netinfo))
				break;
			count -= sizeof (struct netinfo);

#if BSD < 198810
			if (sizeof(n->rip_dst.sa_family) > 1)/* XXX */
				n->rip_dst.sa_family =
					ntohs(n->rip_dst.sa_family);
#endif
			n->rip_metric = ntohl(n->rip_metric);
			/* 
			 * A single entry with sa_family == AF_UNSPEC and
			 * metric ``infinity'' means ``all routes''.
			 * We respond to routers only if we are acting
			 * as a supplier, or to anyone other than a router
			 * (eg, query).
			 */
			if (n->rip_dst.sa_family == AF_UNSPEC &&
			    n->rip_metric == HOPCNT_INFINITY && count == 0) {
			    	if (supplier || (*afp->af_portmatch)(from) == 0)
					supply(from, 0, 0, 0);
				return;
			}
			if ((int) n->rip_dst.sa_family < af_max &&
			    afswitch[n->rip_dst.sa_family].af_hash)
				rt = rtlookup(&n->rip_dst);
			else
				rt = 0;
			n->rip_metric = rt == 0 ? HOPCNT_INFINITY :
				min(rt->rt_Metric + 1, HOPCNT_INFINITY);
#if BSD < 198810
			if (sizeof(n->rip_dst.sa_family) > 1)	/* XXX */
			    n->rip_dst.sa_family = htons(n->rip_dst.sa_family);
#endif
			n->rip_metric = htonl(n->rip_metric);
		}
		rip->rip_cmd = RIPCMD_RESPONSE;
		bcopy((char *)rip, packet, size);
		(*afp->af_output)(s, 0, from, size);
		return;

	case RIPCMD_TRACEON:
	case RIPCMD_TRACEOFF:
		/* verify message came from a privileged port */
		if ((*afp->af_portcheck)(from) == 0)
			return;
		if ((ifp = if_iflookup(from)) == 0 || (ifp->int_flags &
		    (IFF_BROADCAST | IFF_POINTOPOINT | IFF_REMOTE)) == 0 ||
		    ifp->int_flags & IFF_PASSIVE) {
			syslog(LOG_ERR, "trace command from unknown router, %s",
			    (*afswitch[from->sa_family].af_format)(from));
			return;
		}
		((char *)rip)[size] = '\0';
		if (rip->rip_cmd == RIPCMD_TRACEON)
			traceon(rip->rip_tracefile);
		else
			traceoff();
		return;

	case RIPCMD_RESPONSE:
		/* verify message came from a router */
		if ((*afp->af_portmatch)(from) == 0)
			return;
		(*afp->af_canon)(from);
		/* are we talking to ourselves? */
		ifp = if_ifwithaddr(from);
		if (ifp) {
			if (ifp->int_flags & IFF_PASSIVE) {
				syslog(LOG_ERR,
				  "bogus input (from passive interface, %s)",
				  (*afswitch[from->sa_family].af_format)(from));
				return;
			}
			rt = rtfind(from);
			if (rt == 0 || ((rt->rt_State & RTS_INTERFACE) == 0) &&
			    rt->rt_Metric >= ifp->int_metric) 
				addrouteforif(ifp);
			else
				rt->rt_timer = 0;
			return;
		}
		/*
		 * Update timer for interface on which the packet arrived.
		 * If from other end of a point-to-point link that isn't
		 * in the routing tables, (re-)add the route.
		 */
		if ((rt = rtfind(from)) &&
		    (rt->rt_State & (RTS_INTERFACE | RTS_REMOTE)))
			rt->rt_timer = 0;
		else if ((ifp = if_ifwithdstaddr(from)) &&
		    (rt == 0 || rt->rt_Metric >= ifp->int_metric))
			addrouteforif(ifp);
		/*
		 * "Authenticate" router from which message originated.
		 * We accept routing packets from routers directly connected
		 * via broadcast or point-to-point networks,
		 * and from those listed in /etc/gateways.
		 */
		if ((ifp = if_iflookup(from)) == 0 || (ifp->int_flags &
		    (IFF_BROADCAST | IFF_POINTOPOINT | IFF_REMOTE)) == 0 ||
		    ifp->int_flags & IFF_PASSIVE) {
			if (bcmp((char *)from, (char *)&badfrom,
			    sizeof(badfrom)) != 0) {
				syslog(LOG_ERR,
				  "packet from unknown router, %s",
				  (*afswitch[from->sa_family].af_format)(from));
				badfrom = *from;
			}
			return;
		}
		size -= 4 * sizeof (char);
		n = rip->rip_nets;
		for (; size > 0; size -= sizeof (struct netinfo), n++) {
			if (size < sizeof (struct netinfo))
				break;
#if BSD < 198810
			if (sizeof(n->rip_dst.sa_family) > 1)	/* XXX */
				n->rip_dst.sa_family =
					ntohs(n->rip_dst.sa_family);
#endif
			n->rip_metric = ntohl(n->rip_metric);
			if ((int) n->rip_dst.sa_family >= af_max ||
			    (afp = &afswitch[n->rip_dst.sa_family])->af_hash ==
			    (int (*)())0) {
				syslog(LOG_INFO,
		"route in unsupported address family (%d), from %s (af %d)\n",
				   n->rip_dst.sa_family,
				   (*afswitch[from->sa_family].af_format)(from),
				   from->sa_family);
				continue;
			}
			if (((*afp->af_checkhost)(&n->rip_dst)) == 0) {
				syslog(LOG_DEBUG,
				    "bad host in route from %s (af %d)\n",
				   (*afswitch[from->sa_family].af_format)(from),
				   from->sa_family);
				continue;
			}
			if (n->rip_metric == 0 ||
			    (unsigned) n->rip_metric > HOPCNT_INFINITY) {
				if (bcmp((char *)from, (char *)&badfrom2,
				    sizeof(badfrom2)) != 0) {
					syslog(LOG_ERR,
					    "bad metric (%d) from %s\n",
					    n->rip_metric,
				  (*afswitch[from->sa_family].af_format)(from));
					badfrom2 = *from;
				}
				continue;
			}
			/*
			 * Adjust metric according to incoming interface.
			 */
			if ((unsigned) n->rip_metric < HOPCNT_INFINITY)
				n->rip_metric += ifp->int_metric;
			if ((unsigned) n->rip_metric > HOPCNT_INFINITY)
				n->rip_metric = HOPCNT_INFINITY;
			rt = rtlookup(&n->rip_dst);
			if (rt == 0 ||
			    (rt->rt_State & (RTS_INTERNAL|RTS_INTERFACE)) ==
			    (RTS_INTERNAL|RTS_INTERFACE)) {
				/*
				 * If we're hearing a logical network route
				 * back from a peer to which we sent it,
				 * ignore it.
				 */
				if (rt && rt->rt_State & RTS_SUBNET &&
				    (*afp->af_sendroute)(rt, from))
					continue;
				if ((unsigned)n->rip_metric < HOPCNT_INFINITY) {
				    /*
				     * Look for an equivalent route that
				     * includes this one before adding
				     * this route.
				     */
				    rt = rtfind(&n->rip_dst);
				    if (rt && equal(from, &rt->rt_Router))
					    continue;
				    rtadd(&n->rip_dst, from, n->rip_metric, 0,
					  RTP_RIP);
				    changes++;
				}
				continue;
			}

			/*
			 * Update if from gateway and different,
			 * shorter, or equivalent but old route
			 * is getting stale.
			 */
			if (equal(from, &rt->rt_Router)) {
				if (n->rip_metric != rt->rt_Metric) {
					rtchange(rt, from, n->rip_metric,
						 RTP_RIP);
					changes++;
					rt->rt_timer = 0;
					if (rt->rt_Metric >= HOPCNT_INFINITY)
						rt->rt_timer =
						    GARBAGE_TIME - EXPIRE_TIME;
				} else if (rt->rt_Metric < HOPCNT_INFINITY)
					rt->rt_timer = 0;
			} else if ((unsigned) n->rip_metric < rt->rt_Metric ||
			    (rt->rt_Metric == n->rip_metric &&
			    rt->rt_timer > (EXPIRE_TIME/2) &&
			    (unsigned) n->rip_metric < HOPCNT_INFINITY)) {
				rtchange(rt, from, n->rip_metric, RTP_RIP);
				changes++;
				rt->rt_timer = 0;
			}
		}
		break;
	}

	/*
	 * If changes have occurred, and if we have not sent a broadcast
	 * recently, send a dynamic update.  This update is sent only
	 * on interfaces other than the one on which we received notice
	 * of the change.  If we are within MIN_WAITTIME of a full update,
	 * don't bother sending; if we just sent a dynamic update
	 * and set a timer (nextbcast), delay until that time.
	 * If we just sent a full update, delay the dynamic update.
	 * Set a timer for a randomized value to suppress additional
	 * dynamic updates until it expires; if we delayed sending
	 * the current changes, set needupdate.
	 */
	if (changes && supplier &&
	   now.tv_sec - lastfullupdate.tv_sec < SUPPLY_INTERVAL-MAX_WAITTIME) {
		u_long delay;
#ifdef SYSV
		extern long lrand48();
#else
		extern long random();
#endif

		if (now.tv_sec - lastbcast.tv_sec >= MIN_WAITTIME &&
		    timercmp(&nextbcast, &now, <)) {
			if (traceactions)
				fprintf(ftrace, "send dynamic update\n");
			toall(supply, RTS_CHANGED, ifp);
			lastbcast = now;
			needupdate = 0;
			nextbcast.tv_sec = 0;
		} else {
			needupdate++;
			if (traceactions)
				fprintf(ftrace, "delay dynamic update\n");
		}
#ifdef SYSV
#define RANDOMDELAY()	(MIN_WAITTIME * 1000000 + \
		(u_long)lrand48() % ((MAX_WAITTIME - MIN_WAITTIME) * 1000000))
#else
#define RANDOMDELAY()	(MIN_WAITTIME * 1000000 + \
		(u_long)random() % ((MAX_WAITTIME - MIN_WAITTIME) * 1000000))
#endif

		if (nextbcast.tv_sec == 0) {
			delay = RANDOMDELAY();
			if (traceactions)
				fprintf(ftrace,
				    "inhibit dynamic update for %d usec\n",
				    delay);
			nextbcast.tv_sec = delay / 1000000;
			nextbcast.tv_usec = delay % 1000000;
			timevaladd(&nextbcast, &now);
			/*
			 * If the next possibly dynamic update
			 * is within MIN_WAITTIME of the next full update,
			 * force the delay past the full update,
			 * or we might send a dynamic update just before
			 * the full update.
			 */
			if (nextbcast.tv_sec > lastfullupdate.tv_sec +
			    SUPPLY_INTERVAL - MIN_WAITTIME)
				nextbcast.tv_sec = lastfullupdate.tv_sec +
				    SUPPLY_INTERVAL + 1;
		}
	}
}

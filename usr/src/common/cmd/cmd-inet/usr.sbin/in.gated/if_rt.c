/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.gated/if_rt.c	1.2"
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
 *  $Header: /nfs/chumley/usr.src/devel/gated/dist/src/RCS/if_rt.c,v 2.0.1.2 90/10/23 14:00:54 jch Exp $
 */

/********************************************************************************
*										*
*	GateD, Release 2							*
*										*
*	Copyright (c) 1990 by Cornell University				*
*	    All rights reserved.						*
*										*
*	    Royalty-free licenses to redistribute GateD Release 2 in		*
*	    whole or in part may be obtained by writing to:			*
*										*
*	    Center for Theory and Simulation in Science and Engineering		*
*	    Cornell University							*
*	    Ithaca, NY 14853-5201.						*
*										*
*	THIS SOFTWARE IS PROVIDED "AS IS" AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
*										*
*	GateD is based on Kirton's EGP, UC Berkeley's routing daemon		*
*	(routed), and DCN's HELLO routing Protocol.  Development of Release	*
*	2 has been supported by the National Science Foundation.		*
*										*
*	The following acknowledgements and thanks apply:			*
*										*
*	    Mark Fedor (fedor@psi.com) for the development and maintenance	*
*	    up to release 1.3.1 and his continuing advice.			*
*										*
*********************************************************************************
*      Portions of this software may fall under the following			*
*      copyrights: 								*
*										*
*	Copyright (c) 1988 Regents of the University of California.		*
*	All rights reserved.							*
*										*
*	Redistribution and use in source and binary forms are permitted		*
*	provided that the above copyright notice and this paragraph are		*
*	duplicated in all such forms and that any documentation,		*
*	advertising materials, and other materials related to such		*
*	distribution and use acknowledge that the software was developed	*
*	by the University of California, Berkeley.  The name of the		*
*	University may not be used to endorse or promote products derived	*
*	from this software without specific prior written permission.		*
*	THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR		*
*	IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED		*
*	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.	*
********************************************************************************/


/*
 *  Routines for handling routes to interfaces
 */

#include "include.h"

int if_rtactive = FALSE;		/* Set to true if any protocol is broadcasting */

 /*******************************************************************************/
 /*  if_rtdown updates the routing table when an interface has transitioned	*/
 /*  down.  It is also used at init when an interface is found to be down	*/
 /*  by having init_flag be TRUE.						*/
 /*										*/
 /*  At init time, routes to direct interfaces are assumed to be		*/
 /*  RTPROTO_KERNEL as read from the kernel's routing table.  When an		*/
 /*  interface transitions down routes are assumed to be RTPROTO_DIRECT.	*/
 /*										*/
 /*  If the interface is point-to-point, the hostroute to the other side	*/
 /*  of the link is deleted.  If not, the route to the attached interface	*/
 /*  is deleted.  If the interface is subnetted, the internal route to		*/
 /*  the whole net is deleted if this interface is the gateway.			*/
 /*******************************************************************************/
void
if_rtdown(ifp)
if_entry *ifp;
{
    sockaddr_un dst;
    rt_entry *rt;

    if (ifp->int_state & IFS_LOOPBACK) {
	/*  Delete or declare unreachable route to loopback host */
	if (rt = rt_locate(RTS_HOSTROUTE, &ifp->int_addr, RTPROTO_DIRECT)) {
	    (void) rt_unreach(rt);
	}
    } else if (ifp->int_state & IFS_POINTOPOINT) {
	/*  Delete or declare unreachable route to host at other end of point-to-point link */
	if (rt = rt_locate(RTS_HOSTROUTE, &ifp->int_dstaddr, RTPROTO_DIRECT)) {
	    (void) rt_unreach(rt);
	}
    } else {
	/*	Delete or declare unreachable route to subnet and full net */
	sockclear_in(&dst);

	if (rt = rt_locate(RTS_INTERIOR, &ifp->int_subnet, RTPROTO_DIRECT)) {
	    if (rt->rt_ifp == ifp) {
		(void) rt_unreach(rt);
	    }
	}
	if (ifp->int_state & IFS_SUBNET) {
	    if (rt = rt_locate(RTS_INTERIOR, &ifp->int_net, RTPROTO_DIRECT)) {
		if (rt->rt_ifp == ifp) {
		    (void) rt_unreach(rt);
		}
	    }
	}
    }
}


 /*******************************************************************************/
 /*	if_rtbest() is used to make sure the route to an interface is the one	*/
 /*	with the best metric.							*/
 /*******************************************************************************/
static void
if_rtbest(ifp, dst, mask, iflags)
if_entry *ifp;
sockaddr_un *dst;
sockaddr_un *mask;
flag_t iflags;
{
    rt_entry *rt;

    if (rt = rt_locate(RTS_INTERIOR, dst, RTPROTO_DIRECT)) {
	/* Route exists */
	if ((ifp->int_metric < rt->rt_ifp->int_metric) || (rt->rt_state & RTS_HOLDDOWN)) {
	    /*  This interface is the most attractive route, update existing route  */
	    rt->rt_state = (rt->rt_state & ~RTS_NOAGE) | (iflags & RTS_NOAGE);
	    (void) rt_change(rt,
			     &ifp->int_addr,
			     ifp->int_metric,
			     (time_t) 0,
			     ifp->int_preference);
	}
    } else {
	/*  No route to the net exists, add an interface route to our tables only  */
	(void) rt_add(dst,
		      mask,
		      &ifp->int_addr,
		      (gw_entry *) 0,
		      ifp->int_metric,
		      iflags | RTS_INTERIOR,
		      RTPROTO_DIRECT,
		      0,
		      (time_t) 0,
		      ifp->int_preference);
    }
}


 /*******************************************************************************/
 /*  if_rtup processes an interface transition to up or an interface that	*/
 /*  is up at init time.							*/
 /*										*/
 /*  If the interface is point-to-point, any host route to that interface	*/
 /*  is deleted and a host route to the interface with a protocol of		*/
 /*  RTPROTO_DIRECT is added to the routing table.				*/
 /*										*/
 /*  For non-point-to-point interfaces, the route to the attached network	*/
 /*  is deleted from the routing table and a RTPROTO_DIRECT route is		*/
 /*  added. 									*/
 /*										*/
 /*  If non-point-to-point subnetted interfaces, the routing table is		*/
 /*  searched for a direct route to this interface				*/
 /*******************************************************************************/
void
if_rtup(ifp)
if_entry *ifp;
{
    sockaddr_un dst;
    rt_entry *rt;
    flag_t iflags = 0;

    if (ifp->int_state & IFS_NOAGE) {
	iflags |= RTS_NOAGE;
    }
    if (ifp->int_state & IFS_LOOPBACK) {
	/*  Delete all routes to the loopback host and then add a host route to it */
	if (rt = rt_locate(RTS_HOSTROUTE, &ifp->int_addr, RTPROTO_DIRECT)) {
	    (void) rt_unreach(rt);
	}
	(void) rt_add(&ifp->int_addr,
		      &ifp->int_subnetmask,
		      &ifp->int_addr,
		      (gw_entry *) 0,
		      ifp->int_metric,
		      RTS_HOSTROUTE | iflags,
		      RTPROTO_DIRECT,
		      0,
		      (time_t) 0,
		      ifp->int_preference);
    } else if (ifp->int_state & IFS_POINTOPOINT) {
	/*  Delete all routes to the destination of this interface */
	iflags |= RTS_POINTOPOINT;
	if (rt = rt_locate(RTS_HOSTROUTE, &ifp->int_dstaddr, RTPROTO_DIRECT)) {
	    (void) rt_unreach(rt);
	}
	/* Add a route to the interface.  For a P2P link, the gateway	*/
	/* should be the dstaddr, not the interface address. */
	(void) rt_add(&ifp->int_dstaddr,
		      &ifp->int_subnetmask,
		      &ifp->int_dstaddr,/* Gateway is destination */
		      (gw_entry *) 0,
		      ifp->int_metric,
		      RTS_HOSTROUTE | iflags,
		      RTPROTO_DIRECT,
		      0,
		      (time_t) 0,
		      ifp->int_preference);
    } else {
	/*  Delete any routes to this subnet and add an interface route to  */
	/*  it if we are the most attractive.                               */
	sockclear_in(&dst);
	if_rtbest(ifp, &ifp->int_subnet, &ifp->int_subnetmask, iflags | ((ifp->int_state & IFS_SUBNET) ? RTS_SUBNET : 0));
	iflags |= RTS_NOTINSTALL;
	if (ifp->int_state & IFS_SUBNET) {
	    /*  Interface is to a subnet.  Must update route to the main net  */
	    /*  if this is the most attractive interface to it.               */
	    if_rtbest(ifp, &ifp->int_net, &ifp->int_netmask, iflags);
	}
    }
}


 /*******************************************************************************/
 /*  if_rtupdate() is used when a routing packet is received from an		*/
 /*  interface to make sure that a route to this interface exists.		*/
 /*										*/
 /*  If the route to this interface exists, it's metric is set to the		*/
 /*  interface metric of this interface and it's timer is reset.  If this	*/
 /*  is a subnet route, the internal route to the main net is also updated.	*/
 /*										*/
 /*  If the route to this interface does not exist, if_rtup is called to	*/
 /*  add it to the routing table.						*/
 /*******************************************************************************/
void
if_rtupdate(ifp)
if_entry *ifp;
{
    int open = FALSE;
    rt_entry *rt;
    sockaddr_un dst;

    if (ifp->int_state & IFS_LOOPBACK) {
	/*  Refresh the loopback route */
	if ((rt = rt_locate(RTS_HOSTROUTE, &ifp->int_addr,
			    RTPROTO_DIRECT)) && (rt->rt_ifp == ifp) && (ifp->int_state & IFS_UP)) {
	    open = TRUE;
	    rt_open(if_task);
	    if (rt->rt_metric != ifp->int_metric) {
		(void) rt_change(rt,
				 &rt->rt_router,
				 ifp->int_metric,
				 rt->rt_timer_max,
				 ifp->int_preference);
	    } else {
		rt_refresh(rt);
	    }
	} else {
	    if_check(if_task->task_timer[0], (time_t) 0);
	}
    } else if (ifp->int_state & IFS_POINTOPOINT) {
	/*  Refresh a pointopoint route */
	if ((rt = rt_locate(RTS_HOSTROUTE, &ifp->int_addr,
			    RTPROTO_DIRECT)) && (rt->rt_ifp == ifp) && (ifp->int_state & IFS_UP)) {
	    open = TRUE;
	    rt_open(if_task);
	    if (rt->rt_metric != ifp->int_metric) {
		(void) rt_change(rt,
				 &rt->rt_router,
				 ifp->int_metric,
				 rt->rt_timer_max,
				 ifp->int_preference);
	    } else {
		rt_refresh(rt);
	    }
	} else {
	    if_check(if_task->task_timer[0], (time_t) 0);
	}
    } else {
	/*  If the route to this (sub)net is ours, refresh it.  */
	sockclear_in(&dst);
	if ((rt = rt_locate(RTS_NETROUTE, &ifp->int_subnet, RTPROTO_DIRECT)) &&
	    (rt->rt_ifp == ifp) && (ifp->int_state & IFS_UP)) {
	    open = TRUE;
	    rt_open(if_task);
	    if (rt->rt_metric != ifp->int_metric) {
		(void) rt_change(rt,
				 &rt->rt_router,
				 ifp->int_metric,
				 rt->rt_timer_max,
				 ifp->int_preference);
	    } else {
		rt_refresh(rt);
	    }
	} else {
	    if_check(if_task->task_timer[0], (time_t) 0);
	}

	if (ifp->int_state & IFS_SUBNET) {
	    /*  Interface is to a subnet.  Must update route to the main net  */
	    /*  if this is the most attractive interface to it.               */

	    if (!open) {
		open = TRUE;
		rt_open(if_task);
	    }
	    if ((rt = rt_locate(RTS_NETROUTE, &ifp->int_net, RTPROTO_DIRECT)) &&
		(rt->rt_ifp == ifp)) {
		if (rt->rt_metric != ifp->int_metric) {
		    (void) rt_change(rt,
				     &rt->rt_router,
				     ifp->int_metric,
				     rt->rt_timer_max,
				     ifp->int_preference);
		} else {
		    rt_refresh(rt);
		}
	    } else {
		/* Should never happen */
		if_rtup(ifp);
	    }
	}
    }

    if (open && rt_close(if_task, (gw_entry *) 0, 0)) {
	task_flash(if_task);
    }
}


 /*******************************************************************************/
 /* if_rtinit() initializes the interior routing table with direct nets as	*/
 /* per the interface table. Such routes read from the kernel routing tables	*/
 /* are deleted from the exterior routing table.				*/
 /*******************************************************************************/
void
if_rtinit()
{
    register if_entry *ifp;

    trace(TR_RT, 0, NULL);
    trace(TR_RT, 0, "if_rtinit: interior routes for direct interfaces:");

    rt_open(if_task);

    IF_LIST(ifp) {
	/* If this interface can not hear it's own packets, or we are not actively participating in a routing protocol, */
	/* mark the interface as passive. */
	if ((ifp->int_state & IFS_SIMPLEX) ||
	    ((ifp->int_state & (IFS_NORIPIN | IFS_NORIPOUT)) && (ifp->int_state & (IFS_NOHELLOIN | IFS_NOHELLOOUT))) ||
	    !if_rtactive) {
	    ifp->int_state |= IFS_NOAGE;
	    trace(TR_INT, 0, "if_rtinit: interface %s: %A marked passive",
		  ifp->int_name,
		  &ifp->int_addr);
	}
	if (ifp->int_state & IFS_UP) {
	    if_rtup(ifp);
	} else {
	    if_rtdown(ifp);
	}
    } IF_LISTEND;

    rt_close(if_task, (gw_entry *) 0, n_interfaces + 1);
}

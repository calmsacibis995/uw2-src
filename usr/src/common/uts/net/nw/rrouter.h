/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_RROUTER_H  /* wrapper symbol for kernel use */
#define _NET_NW_RROUTER_H  /* subject to change without notice */

#ident	"@(#)kern:net/nw/rrouter.h	1.6"
#ident	"$Id: rrouter.h,v 1.6.2.1 1994/10/24 19:46:02 vtag Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
*/

/* rrouter.h
**
** A description of the encapsulated data and operations
** constituting the interface
** 	1) Provided by a Replaceable Router and consumed by IPX.
**	2) Provided by IPX and consumed by a Replaceable Router.
**
** The intent here is to provide an interface and abstraction
** tools general enough for IPX to accomodate generic routers,
** while allowing IPX and a Replaceable Router to hide their
** implementation details from each other (for their mutual
** protection and benefit).  This is important for all the
** "proper" reasons, but particularly (and a driving motivation
** for the degree of abstraction given here) for multi-threaded
** pre-emptive operating systems and symmetric multi-processors.
** This has been done with little or no impact on efficiency
** of code paths that bear heavy traffic. IPX and a router
** should not intrude on or use each others data except via
** provided methods.
*/
#ifdef _KERNEL_HEADERS
#include <net/nw/lipmx_app.h>
#include <net/nw/nwcommon.h>
#else
#include <sys/lipmx_app.h>
#include <nwcommon.h>
#endif /* _KERNEL_HEADERS */

/***************************************************************
** SECTION 1:
**	Data and methods provided *to* IPX *by* Replaceable Routers.
**
**  The functions prototyped below (needed by IPX) are made
**	available to IPX by stuffing their pointers into a struct
**	and passing its pointer to IPXRegisterRRouter. The router
**	should include this header and implement the functions in
**	this Section (1).
**
**	The functions not used by IPX (not defined in
**	RRouterEntryPoints_t) need to implemented as IOCTLs.
****************************************************************
*/

/* IPX solicits routing info from the router by passing
** a pointer to an ipxHdr_t and a void pointer to an ipxLanKey.
**	-  the router allocates a link layer header (if
** appropriate to the gateway lan) and points the ipxLanKeyPtr
** to the correct ipxLanKey (representing the lan the packet
** should go out on).
**		mblk_t	*(*GetRouteInfo)(ipxHdr_t *, void **ipxLanKeyPtr);
*/

/* IPX and a replaceable router usually transact business with
** each other using a complementary pair of keys (/tokens/handles),
** which both abstractly represent a given lan.  IPX represents a
** lan to the replaceable router via an ipxLanKey.  The replaceable
** router represents a lan to IPX via an rrLanKey.
** IPX obtains, queries the validity of, and relenquishes
** rrLanKeys via methods provided by the router. The router
** casts a key to whatever form is convenient, and maintains
** control over the data and methods the key accesses.
**
** The functions described here and declared below must be
** implemented by a router driver, then registered with IPX
** via IPXRegisterRRouter() as in Section 2 below.
**
**void	*(*GrantLanKey)(rrLanData_t *lanData);
** GrantLanKey() is passed a pointer to information
** IPX is willing to make available to the router. The router
** allocates its own structure, copies any information passed
** in that it finds useful, and adds any other info it may
** need to help manage requests. IPX doesn't see any of this -
** it accesses the router blindly via the rrLanKey.
**
**void	(*UpdateLanKey)(void *rrLanKey, rrLanData_t *lanData);
** UpdateLanKey() allows IPX to notify the router that a limited
** subset of the information it passed in GrantLanKey() is changed.

**void	*(*InvalidateLanKey)(void *rrLanKey);
** InvalidateKey() lets IPX notify the router that the
** the LAN represented by the ipxLankey/rrLanKey pair is being
** de-configured, and the router should take whatever
** action internally that it deems appropriate. It then
** returns a NULL pointer. It may remove the network represented
** by the key from its tables, notify other networks of the
** change, or ignore it - what it does doesn't matter to IPX
** as long as IPX can reliably solicit accurate routing info
** when it needs to.
**
**void	(*DigestRouterPacket)(void *rrLanKey, mblk_t *mp);
** DigestRouterPacket() is called when IPX receives a RIP packet
** on a connected lan.  The router can use this information 
** to maintain its router information.  The packet is guaranteed
** contiguous.  The packet is only "loaned" to the router - the
** router can reference the information, baut must not change or
** discard the packet.
**
** ??? With proper abstraction in LIPMX, this can be eliminated ???
**int	(*CheckSapPacket)(void *rrLanKey, mblk_t *mp);
** CheckSapPacket() should check that a SAP packet is legal and
** makes sense - ie; is not from a remote net, etc.
**
**int	(*MapNetToIpxLanKey)(uint32 targetNet, void **ipxLanKeyPtr);
** MapNetToIpxLanKey() allows a router to find out from IPX the
** network number associated with an ipxLanKey.
**
**mblk_t	*(*GetRouteInfo)(ipxHdr_t *destination, void **ipxLanKeyPtr)
** GetRouteInfo() is used by IPX to get routing information for
** a given destination address.  If a route is known to the given
** network, the router (as a side-effect) points ipxLanKeyPtr
** to IPX's private ipxLanKey representing the lan leading to the
** destination address.
*/
typedef struct	rrLanData {
	void	*ipxLanKey;	/* unique lan identifier used by IPX */
	uint32	ipxNetAddr;		/* IPX network number */
	ipxNode_t	ipxLanNode;	/* IPX node addr of lan on ipxNetAddr */
	uint16	lanTicks;		/* lan speed in ticks */
	uint8	lanType;	/* WAN, STAR, ... */
	mblk_t	*prepend;	/* link layer header (if any) for lan */
	uint32	addrOffset;	/* where in the LLhdr to poke the destNode */
	RIPperLanInfo_t	ripCfg;	/* per-lan configurable info */
} rrLanData_t;

typedef struct	RROUTERMethods {
	void	*(*GrantLanKey)(rrLanData_t *lanData);
	void	 (*UpdateLanKey)(void *rrLanKey, rrLanData_t *lanData);
	void	*(*InvalidateLanKey)(void *rrLanKey);
	void	 (*DigestRouterPacket)(void *rrLanKey, mblk_t *mp);
#ifdef OLD_SAP_CHECKING
	int		 (*CheckSapPacket)(void *rrLanKey, mblk_t *mp);
#endif
	int		 (*MapNetToIpxLanKey)(uint32 targetNet, void **ipxLanKeyPtr);
	mblk_t	*(*GetRouteInfo)(ipxHdr_t *, void **ipxLanKeyPtr);
} RROUTERMethods_t;
extern RROUTERMethods_t	RROUTER;

/* IOCTL functions - opens on /dev/<rrouter> and subsequent
** ioctl() calls give access to these methods.
*/
/* clean these up - functional descr.??? */
/* The following types of IOCTLs and the described functionality
** should be supported by routers.
**#define IPX_START_RROUTER (or do this on control dev open ?)
** Might include some configuration info.  When the ioctl is
** acked, the router should be up and running.
**
**#define IPX_RESET_RROUTER
** Flush routing tables, keeping only the information for locally
** connected lans (ie; info tied to rrLanKeys).
**
**#define IPX_STOP_RROUTER (or on control dev close?)
** Stop doing routing, and deregister from IPX.
**
**#define IPX_RROUTER_GET_NET_INFO
** Return routing information for a given network.
**
**#define IPX_RROUTER_GET_ROUTER_TABLE
** Dump the entire routing table.
**
**#define IPX_RROUTER_CHECK_SAP_SOURCE
** Check the given information about a SAP source.  Is the
** source a sane and legal source of SAP info?
*/

/***************************************************************
** SECTION 2:
**	Data and methods provided *to* Replaceable Routers *by* IPX.
**
**  The functions prototyped here are available to routers
**	by virtue of the way drivers typically work - a daemon
**	assembles the protocol stack according to configurable
**	parameters, one of which is the name of the /dev/<rrouter>
**  device representing the router. The driver is loaded after the
**	IPX devices (and depends on a symbol in IPX - RRIPX), and so
**  the IPX-provided functions and data are made available to the
**  router by the (possibly dynamic) linker.
****************************************************************
*/

/* SendData flags - should the message be sent paced or not?
*/
#define	RR_DIRECT	0x00
#define RR_NOPACE	0x00	/* Same meaning as DIRECT */
#define	RR_BCAST	0x01
#define RR_PACE		0x02
#define RR_QUEUE	0x04

/* IpxLanKey with this value represents all locally connected lans
*/
#define ALL_LANS	(void *)-1
#define ALL_NETS	ALL_LANS

typedef struct RRIPXMethods {
	void	*(*RegisterRRouter)(RROUTERMethods_t *rrouterEntryTable);
	int		 (*DeregisterRRouter)(void *rrouterToken);
	void	 (*SendData)(mblk_t *, void **ipxLanKeyPtr, uint8 sndFlags);
	uint32	 (*MapSapLanToNetwork)(uint32 sapConnectedLan);

	/* If ipxLanKey == ALL_LANS, cycle through
	** lans/rrLanKeys calling method
 	*/
	void	*(*UseMethodWithLanKey)(void *ipxLanKey, void *rrPrivateKey,
								void *rrPrivateMethod(void *rrPrivateKey,
													void *rrLanKey));
	atomic_int_t	Ripcount;		/* Current count of active calls into 
									** router.
									*/
} RRIPXMethods_t;
extern RRIPXMethods_t	RRIPX;

/*    Router             |          IPX
**      |                |
**      o-----------ipxLanKey---> IPXUseMethodWithLanKey()
**                       |                        | +rrLanKey
**    rrPrivateMethod()<--rrPrivateKey + rrLanKey-o
*/

#endif	/* _NET_NW_RROUTER_H */

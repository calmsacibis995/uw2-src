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

#ident	"@(#)kern:net/nw/ripx/rip.c	1.18"
#ident	"$Id: rip.c,v 1.29.2.1 1994/10/24 19:48:32 vtag Exp $"

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
/*
 * rip.c
 *	The Routing Information Table (RIT) is entirely encapsulated
 *	within this module.
 *
 *  Maintain and support queries to the RIT.  This Maintenance
 *	includes sending appropriate updates out attached LANs.
 *
 * Lock usage - Code/structures protected:
 *  RipNetHash
 */
#ifdef _KERNEL_HEADERS
#include <net/nw/ripx/rip.h>
#include <net/nw/ripx/ripx_streams.h>
#include <net/nw/ripx/rrrip.h>
#include <net/nw/rrouter.h>
#else
#include "rip.h"
#include "ripx_streams.h"
#include "rrrip.h"
#include "rrouter.h"
#endif /* _KERNEL_HEADERS */

/*
 * Forward References for Router Functions
 */
FSTATIC netListEntry_t	*RipGetNetEntry(uint32);
FSTATIC	int		 RIPSetRITHashSize(uint16);
FSTATIC void	 RipAcceptRouterInfo(void *, routePacket_t *, uint16);
FSTATIC void	 RipAddNetRoute(netListEntry_t *, netRouteEntry_t *);
FSTATIC void	 RipAdvertise(void);
FSTATIC void	 RipAgeRouters(long);
FSTATIC void	 RipGiveRouterInfo(void *, mblk_t *);
FSTATIC void	 RipInitRouter(void);
FSTATIC uint32	 RipKillNetEntry(netListEntry_t *, RtInfoTable_t *);
FSTATIC void	 RipPostLostRoutes(void);
FSTATIC void	 RipRemoveNetRoute(netListEntry_t *, netRouteEntry_t *);
FSTATIC void	*RipRequestRouteInfo(void *, void *);
FSTATIC uint16	 RoundToPowerOf2(uint16);
FSTATIC uint16	 RipSetMaxHops(uint16 maxHops);
#define	SYSTEM	(void *)0
#define	DIRECT	(void *)1
FSTATIC	void	 RipScheduleRip(void *);

static NetHash_t	DefaultHash = {0};
RtInfoTable_t	DefaultRIT = { 0, &DefaultHash, 0, {0}};
RtInfoTable_t	RIT = { 0, NULL, 0, {0}};
#define HASH_LOCK	32
static LKINFO_DECL(RipHashLkInfo, "RipxHashLock", 0);

#define SAVE_LOCALS 0
#define KILL_LOCALS 1
FSTATIC void	 RipClearRIT(RtInfoTable_t *, int);
FSTATIC void	 RipKillTableAndRips(RtInfoTable_t *);

#define RIP_NET_HASH(x) (((*((unsigned short *)(x))) \
							+ (*((unsigned short *)(x) + 1)) \
						 ) & RIT.mask)

#define ROUTE_REQUEST	1
#define ROUTE_RESPONSE	2
#define CHANGED	(uint8)0x01
#define	DEFAULT_MAX_HOPS	16
uint16	MaxHops = DEFAULT_MAX_HOPS;
uint8   routerType = 0;

extern	uint8   ripControlClosing;

/*
 *	Statistics Variables
 */
/* re-zeroed when driver reloaded */
RouterInfo_t ripStats = { 0 };

/*
 * The following global variables are used by the router.
 */
static time_t	lastRouterAgedTime = 0;
int	BasisMultiplier = 1;
/* basis of update (in ticks) */
static long	TicksPerBasis;
static long	UpdateTicks;

static int		ripFlag = FALSE;
static toid_t	ripId = 0;
static queue_t	*RipQ = NULL;
static int8		ageNets = FALSE;
static int8		lostNets = FALSE;
static int8		foundNets = FALSE;
const uint16 RipSocket = GETINT16(RIP_SOCKET);
const uint8	AllHosts[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static const uint8	*AllNets = AllHosts;

/*
 * void ripxinit(void)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
int
ripxinit(void)
{
	NTR_ENTER(0, 0,0,0,0,0);

	/*
	 *+ Inform that driver is loaded
	 */
#ifdef DEBUG
	cmn_err(CE_CONT, "%s %s%s: %s %s\n",
						RIPXSTR, RIPXVER, RIPXVERM, __DATE__, __TIME__);
#else
	cmn_err(CE_CONT, "%s %s%s\n", RIPXSTR, RIPXVER, RIPXVERM);
#endif /* DEBUG */

	ripStats.RipxMajorVersion = (uint8)RIPX_MAJOR;
	ripStats.RipxMinorVersion = (uint8)RIPX_MINOR;
	ripStats.RipxRevision[0] = (char)RIPX_REV1;
	ripStats.RipxRevision[1] = (char)RIPX_REV2;

	if ((DefaultHash.lock
				= LOCK_ALLOC(HASH_LOCK, plstr, &RipHashLkInfo,
								KM_NOSLEEP)) == NULL) {
		/*
		 *+ Could not allocate lock structure
		 */
		cmn_err(CE_WARN, "ripxinit: lock alloc failure");
		return(NTR_LEAVE( 0 ));
	}
	TicksPerBasis = 30 * HZ;
	UpdateTicks = 30 * HZ;
	return(NTR_LEAVE(ripxAllocStreamsLocks()));
}

/*
 * int ripxfini(void)
 *	All done, deallocate locks
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
int
ripxfini(void)
{
	NTR_ENTER(0, 0,0,0,0,0);

#ifdef DEBUG
	if (DefaultRIT.table->list) {
		/*
		 *+ DEBUG, RIT is not empty on shutdown
		 */
		cmn_err(CE_WARN, "ripxfini: hash entries in DefaultRIT !");
	}
#endif /* def DEBUG */
	LOCK_DEALLOC(DefaultHash.lock);
	ripxDeallocStreamsLocks();
	return(NTR_LEAVE( 0 ));
}

/*
 * void RipAdjustTimerInterval(uint16 multiplier)
 *
 *
 * Calling/Exit State:
 *  Called when a lan is having a rrLanKey built for it.
 *  This happens as a result of ioctls on the control device
 *  of lipmx (serialized) or registering the router (lipmx
 *  registers lans serially).
 *
 * Lock usage - Code/structures protected:
 *  None.
 */
void
RipAdjustTimerInterval(uint16 multiplier)
{ static uint16	max = 0;
		uint16	new;

	NTR_ENTER(1, multiplier, 0,0,0,0);

	if (multiplier != 0) {
		/* if first time, prime the pump */
		if (max == 0)
			BasisMultiplier = multiplier;
		if (multiplier > max)
			max = multiplier;
		for (new = (int)BasisMultiplier; new >= 1; new--)
			if ((multiplier % new) == 0)
				break;
		BasisMultiplier = new;
		UpdateTicks = BasisMultiplier * TicksPerBasis;
		NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"AdjustTimerIntvl: BasisMultiplier = %d, UpdateTicks = %d",
			BasisMultiplier, UpdateTicks));
	}
	NTR_VLEAVE();
	return;
}

/*
 * void RipInitRouter(void)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipInitRouter(void)
{
	NTR_ENTER(0, 0,0,0,0,0);

	ripFlag = FALSE;

	NTR_VLEAVE();
	return;
}

/*
 * void RIPClearChanged(void)
 *
 * Calling/Exit State:
 *  NO LOCKS
 *
 * Lock usage - Code/structures protected:
 *  RIT.table[hIdx].lock
 */
void
RIPClearChanged(void)
{
	pl_t				pl;
	netListEntry_t		*net;
	int					i;

	NTR_ENTER(0, 0, 0,0,0,0);

	for (i = 0; i < RIT.size; i++) {
		pl = LOCK(RIT.table[i].lock, plstr);
		net = RIT.table[i].list;
		while (net != NULL) {
			net->entryChanged = 0;
			net->mustAdvertise = 0;
			net = net->hash_next;
		}
		UNLOCK(RIT.table[i].lock, pl);
	}
	NTR_VLEAVE();
	return;
}

/*
 * netListEntry_t * RipGetNetEntry(uint32 targetNet)
 *
 * Calling/Exit State:
 *  BUCKET LOCK HELD AROUND CALL
 *
 * Lock usage - Code/structures protected:
 *  RIT.table[hIdx].lock
 */
netListEntry_t *
RipGetNetEntry(uint32 targetNet)
{	netListEntry_t		*net;

	NTR_ENTER(1, GETINT32(targetNet), 0,0,0,0);

	net = RIT.table[RIP_NET_HASH(&targetNet)].list;

	/* could just calculate theoretical #'s and not
	** maintain actual stats
	*/
	++RIT.stats.accesses;
	while ((net != NULL) && (net->netIDNumber != targetNet)) {
		++RIT.stats.collisions;
		net = net->hash_next;
	}

	if (net == NULL) {
		++RIT.stats.failedFinds;
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RipGetNetEntry: didn't find net 0x%X", GETINT32(targetNet)));
	}
#ifdef RIT_DEBUG
	else {
		netRouteEntry_t	*route;

		route = net->routeListLink;
		if (!route)
			NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
					"Walk: Net 0x%X has no route", GETINT32(targetNet)));
		while (route) {
			NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
					"Walk: Net 0x%X route link 0x%X",
					GETINT32(targetNet), route));
			route = route->nextRouteLink;
		}
	}
#endif /* def RIT_DEBUG */
	return((netListEntry_t *)NTR_LEAVE(net));
}

/*
 * void RipAcceptRouterInfo(void *rrLanKey, routePacket_t *rtPkt,
 *							uint16 length)
 *  A RIP response needs to be examined.  This function will
 *  only look at the data in the packet.  The RIT is not referenced
 *  in this function.
 *
 * Calling/Exit State:
 *  No locks held.  RIT not referenced.
 *
 * Lock usage - Code/structures protected:
 *  RipAddRouterInfo is called, which uses hash locks.
 */
void
RipAcceptRouterInfo(void *rrLanKey, routePacket_t *rtPkt, uint16 length)
{	int					count, deltas;
	routeEntry_t		*route;
	uint16				targetHops;
	uint16				targetTicks;
	uint8				statusFlags;
	uint32				targetNet;
	long				now;

	/* caller guarantees rrLanKey is valid */
	NTR_ENTER(3, rrLanKey, rtPkt, length, 0, 0);

	if (!RIPChkNetConflict(rrLanKey, rtPkt->src.net, rtPkt->dest.net)) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipAcceptRouterInfo: A network conflict exists"));
		NTR_VLEAVE();
		return;
	}
	statusFlags = RIPNetStatus(rrLanKey)
					& (NET_RELIABLE_BIT | NET_WAN_BIT | NET_STAR_BIT);
	count = ((int)length - ROUTE_PACKET_SIZE) / ROUTE_ENTRY_SIZE;
	route = (routeEntry_t *)rtPkt->routeTable;

	deltas = 0;
	while (count-- > 0) {
		GETALIGN16(&route->targetHops, &targetHops);
		targetHops = GETINT16(targetHops);
		GETALIGN16(&route->targetTime, &targetTicks);
		targetTicks = GETINT16(targetTicks);
		if (targetHops && targetTicks) {
			IPXCOPYNET(&(route->targetNet), &targetNet);
			if ((targetNet == 0) || (targetNet == (uint32)ALL_LANS)) {
				NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
					"RipAcceptRouterInfo: Attempt to add bad net number 0x%X",
					GETINT32(targetNet)));
			} else
				deltas += RIPAddRouterInfo(targetNet, rtPkt->src.node,
									targetHops, targetTicks,
									rrLanKey, statusFlags);
		}
		route++;
	}

	/*
	 * advertise any new nets
	 */
	if (deltas) {
		drv_getparm(LBOLT, (void *)&now);
		RIPSendUpdates(TRUE, now);
	}

	NTR_VLEAVE();
	return;
}

/*
 * int RIPAddRouterInfo(uint32 targetNet, uint8 *sourceRouter,
 *							uint16 targetHops, uint16 targetTicks,
 *							void *rrLanKey, uint8 statusFlags)
 *
 * Called when:
 *	(1) A local Lan is being registered
 *	(2) A RIP packet entry needs to be examined as a candidate
 *	    for RIT inclusion.
 *
 * Calling/Exit State:
 *  No locks held on enter/exit.
 *
 * Lock usage - Code/structures protected:
 *  RIT.table[hIdx].list
 */
int
RIPAddRouterInfo(uint32 targetNet, uint8 *sourceRouter,
				uint16 targetHops, uint16 targetTicks,
				void *rrLanKey, uint8 statusFlags)
{	uint32 			hashIndex;
	uint32 			lanNetAddr;
	uint16			maxAge;
	uint8			lNetType;
	void 			*ipxLanKey;
	pl_t			pl;
	netListEntry_t		*net;
	netListEntry_t		*lNet;
	netListEntry_t		*pNet;
	netRouteEntry_t		*route;

	NTR_ENTER(5, GETINT32(targetNet), sourceRouter, targetHops,
				targetTicks, rrLanKey);

#ifdef HARD_DEBUG
	if ((targetNet == 0) || (targetNet == (uint32)ALL_LANS)) {
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
				"RIPAddRouterInfo: Attempt to add bad net number 0x%X",
				GETINT32(targetNet)));
		return(NTR_LEAVE(0));
	}
#endif /* HARD_DEBUG */
	if (targetHops > MaxHops) /* don't go above top count */
		targetHops = MaxHops;

	/* get information on the lan this packet came in on */
	lanNetAddr = RIPLanData(rrLanKey, &ipxLanKey, NULL, &maxAge);
	hashIndex = RIP_NET_HASH(&lanNetAddr);
	pl = LOCK(RIT.table[hashIndex].lock, plstr);
	lNet = RipGetNetEntry(lanNetAddr);

	/* targetTicks must NEVER be 0 when adding info for connected lan */
	if (targetTicks == 0) {		/* time must be at least 1 tick */
		targetTicks = lNet ? lNet->timeToNet : 1;
	}

	if (lNet) {
		lNetType = lNet->netStatus;
	}
	UNLOCK(RIT.table[hashIndex].lock, pl);

	/* See if this net is already in the RIT */
	hashIndex = RIP_NET_HASH(&targetNet);
	pl = LOCK(RIT.table[hashIndex].lock, plstr);
	net = RipGetNetEntry(targetNet);

	if ((statusFlags & NET_LOCAL_BIT) != 0) {
		if (net != NULL) {
			if (net->netStatus & NET_LOCAL_BIT) {
				/*
				 * local nets should be NULLing rrLanKey first
				 * not just updating info
				 *
				 * A driver is trying to re-initialize a connected LAN that I
				 * already have in my tables.  It should first invalidate the
				 * rrLanKey associated with that lan, causing the net to
				 * be removed from the RIT on the next queuerun of ripchain.
				 */
				/*
				 *+ Inform operation dup net addresses
				 */
				cmn_err(CE_WARN, "Abort Re-adding Local Net 0x%X to RIT !!",
					GETINT32(targetNet));
				NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
						"RIPAddRouterInfo: Abort Re-adding Local Net 0x%X to RIT ?",
				GETINT32(targetNet)));
				UNLOCK(RIT.table[hashIndex].lock, pl);
				return(NTR_LEAVE(0));
			} else {
				/*
				 *+ Inform operation dup net addresses
				 */
				cmn_err(CE_WARN,
						"Configuring Ipx Net address 0x%X that is not unique on the internetwork\n",
						GETINT32(targetNet));
				/*
				 *+ Ditto
				 */
				cmn_err(CE_CONT, " My hops/ticks %d/%d, other %d/%d",
					targetHops, targetTicks, net->hopsToNet, net->timeToNet);
				NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
						"RIPAddRouterInfo: Overwrite non-lcal claim for Net 0x%X ?",
				GETINT32(targetNet)));
				net->hopsToNet = (uint8)targetHops;
				net->entryChanged = CHANGED;
				net->timeToNet = targetTicks;
				net->netStatus = statusFlags;
			}
		}
	}

	if (net == NULL) {
		/* I didn't know about this one! */
		if (targetHops >= MaxHops) {
			UNLOCK(RIT.table[hashIndex].lock, pl);
			NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
				"RIPAddRouterInfo: Net 0x%X exceeded max hops %d ",
				GETINT32(targetNet), targetHops));
			/* don't add info for someone down */
			return(NTR_LEAVE(0));
		}
		if (!RIT.size) {
			NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
				"RIPAddRouterInfo: attempt to add to RIT when inactive"));
			UNLOCK(RIT.table[hashIndex].lock, pl);
			return(NTR_LEAVE(0));
		}
		NWSLOG((RIPXID, 0, PNW_ALL, SL_TRACE,
				"RIPAddRouterInfo: adding net 0x%X hops %d ",
				GETINT32(targetNet), targetHops));
		net = (netListEntry_t *) ripx_alloc(sizeof(netListEntry_t), KM_NOSLEEP);
		if (net == NULL) { /* no space */
			NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"RIPAddRouterInfo: alloc block failed (net)"));
			UNLOCK(RIT.table[hashIndex].lock, pl);
			return(NTR_LEAVE(0));
		}
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPAddRouterInfo: KMEM: Allocate net structure at 0x%X, size %d", 
				net, sizeof(netListEntry_t)));

		route = (netRouteEntry_t *) ripx_alloc(sizeof(netRouteEntry_t),
												KM_NOSLEEP);
		if (route == NULL) { /* no space */
			NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"RIPAddRouterInfo: alloc block failed (route)"));
			UNLOCK(RIT.table[hashIndex].lock, pl);
			NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"RIPAddRouterInfo: KMEM: Free net structure at 0x%X, size %d", 
					net, sizeof(netListEntry_t)));
			ripx_free(net, sizeof(netListEntry_t));
			return(NTR_LEAVE(0));
		}
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPAddRouterInfo: KMEM: Allocate route structure at 0x%X, size %d",
				route, sizeof(netRouteEntry_t)));

		net->netIDNumber = targetNet;
		net->hopsToNet = (uint8)targetHops;
		net->entryChanged = CHANGED;
		net->routeListLink = route;
		net->timeToNet = targetTicks;
		net->hash_next = NULL;
		net->hash_prev = NULL;
		if (statusFlags & NET_LOCAL_BIT)
			net->netStatus = statusFlags;
		else net->netStatus = 0;

		route->nextRouteLink = NULL;
		route->timer = maxAge;
		route->rrLanKey = rrLanKey;
		route->routeTime = targetTicks;
		route->routeStatus = statusFlags;
		route->routeHops = (uint8)targetHops;
		IPXCOPYNODE(sourceRouter, route->forwardingRouter);

		/*
		** Now insert the net route entry and net list entry into
		** the net hash table list.
		*/
		pNet = RIT.table[hashIndex].list;
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
				"RIPAddRouterInfo: HashIn: net %x ind %d prev %x ",
				GETINT32(targetNet), hashIndex, pNet));
		RIT.table[hashIndex].list = net;
		net->hash_next = pNet;
		if (pNet) pNet->hash_prev = net;
		net->hash_prev = NULL;

		if ((rrLanKey != NULL) && (statusFlags & NET_LOCAL_BIT)) {
			net->mustAdvertise = -1;
			foundNets = TRUE;
			RipScheduleRip(DIRECT);
		} else {
			net->mustAdvertise = 0;
		}
		NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"RIPAddRouterInfo: Just added net 0x%X",
			GETINT32(targetNet)));
		UNLOCK(RIT.table[hashIndex].lock, pl);

		/* the entry has been processed */
		return(NTR_LEAVE(1));
	}
	if (targetHops >= MaxHops) {
		/*
		** someone else has lost track of this net!
		** Maybe I'll be able to cause alternate route
		** info to get sent out.
		*/
		net->routerLostNetFlag = TRUE;
		lostNets = TRUE;
		RipScheduleRip(DIRECT);
	}

	/*
	** Check the source of this information.  If the connected net
	** the information came from is "inactive", or if it is a star
	** or WAN with better routes already known, ignore it.
	*/
	if ((lNet == NULL)
				|| ((lNetType & (NET_STAR_BIT | NET_WAN_BIT))
			&& (targetTicks > net->timeToNet))) {
		UNLOCK(RIT.table[hashIndex].lock, pl);
		/* star/WAN info not best */
		return(NTR_LEAVE(0));
	}

	/*
	** I know of this network.  Do I know of this router's path?
	*/
	route = net->routeListLink;
	while (route != NULL) {
		if ((route->rrLanKey == rrLanKey) &&
				(IPXCMPNODE(sourceRouter, route->forwardingRouter))) {
			/* I know of this router already */
			if ((targetTicks == route->routeTime)
					&& (targetHops == route->routeHops)) {
				/* nothing has changed */

				route->timer = maxAge;
				NWSLOG((RIPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
					"RIPAddRouterInfo: nothing has changed "));
				UNLOCK(RIT.table[hashIndex].lock, pl);
				return(NTR_LEAVE(0));
			} else {
				/* this router information has changed */
				if (route->routeStatus & NET_LOCAL_BIT) {
					/*
					** SABOTAGE!!!
					** SOMEONE ELSE CLAIMS MY ADDRESS !!!!!!
					*/
					NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
						"RIPAddRouterInfo: someone claims my net 0x%X!!",
						GETINT32(net->netIDNumber)));
					UNLOCK(RIT.table[hashIndex].lock, pl);
					return(NTR_LEAVE(0));
				}
				RipRemoveNetRoute(net, route);
				NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
					"RIPAddRouterInfo: KMEM: Free route structure at 0x%X, size %d",
						route, sizeof(netRouteEntry_t)));
				ripx_free(route, sizeof(netRouteEntry_t));
				break;
			}
		}
		route = route->nextRouteLink;
	}

	/*
	** This is a new route to the target net.
	*/
	if ((targetHops > net->hopsToNet)
			|| (targetHops >= MaxHops)) {
		UNLOCK(RIT.table[hashIndex].lock, pl);
		/* Not "best" known path */
		if (net->hopsToNet >= MaxHops) {
			NotifySapNetDead(targetNet);
		}
		return(NTR_LEAVE(0));
	}

	route = (netRouteEntry_t *)ripx_alloc(sizeof(netRouteEntry_t), KM_NOSLEEP);
	if (route == NULL) {
		UNLOCK(RIT.table[hashIndex].lock, pl);
		return(NTR_LEAVE(0));
	}
	NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
		"RIPAddRouterInfo: KMEM: Allocate route structure at 0x%X, size %d", 
			route, sizeof(netRouteEntry_t)));
	route->rrLanKey = rrLanKey;
	IPXCOPYNODE(sourceRouter, route->forwardingRouter);
	route->routeHops = (uint8)targetHops;
	route->routeTime = targetTicks;
	route->routeStatus = statusFlags;
	route->timer = maxAge;
	RipAddNetRoute(net , route); /* make a note of the new router */
	UNLOCK(RIT.table[hashIndex].lock, pl);

	return(NTR_LEAVE(1));
}

/*
 * void RipDeconfigureLocalLan(void *rrLanKey, uint32 lNetDying)
 *
 * A rrLanKey is being invalidated.
 * A local net is going down.  Announce its demise,
 * prevent further updates, kill all subsidiaries,
 * and remove it from the Table.
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipDeconfigureLocalLan(void *rrLanKey, uint32 lNetDying)
{	uint32	i;
	pl_t	pl;
	netListEntry_t	 	*net;
	netRouteEntry_t	*route;

	NTR_ENTER(2, rrLanKey, GETINT32(lNetDying), 0,0,0);
	NWSLOG((RIPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter RipDeconfigureLocalLan: Down Ipx local lan, Net 0x%X",
			GETINT32(lNetDying)));

	pl = LOCK(RIT.table[RIP_NET_HASH(&lNetDying)].lock, plstr);
	net = RipGetNetEntry(lNetDying);

	if (net == NULL) {
		UNLOCK(RIT.table[RIP_NET_HASH(&lNetDying)].lock, pl);
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"RipDeconfigureLocalLan: rrLanKey 0x%X net 0x%X no entry!!", rrLanKey, GETINT32(lNetDying)));
#ifdef DEBUG
		cmn_err(CE_WARN, "RipDeconfigureLocalLan: rrLanKey 0x%X net 0x%X no entry!!", rrLanKey, GETINT32(lNetDying));
#endif /* def DEBUG */
		NTR_VLEAVE();
		return;
	}
	net->hopsToNet = MAX_MAX_HOPS;
	UNLOCK(RIT.table[RIP_NET_HASH(&lNetDying)].lock, pl);

	for (i = 0; i < RIT.size; i++) {
		pl = LOCK(RIT.table[i].lock, plstr);
		net = RIT.table[i].list;
		while (net != NULL) {
			route = net->routeListLink;
			while (route != NULL) {
				if (route->rrLanKey == rrLanKey) {
					route->routeHops = MAX_MAX_HOPS;
					/* so broadcast will kill it */
					route->timer = 0;	/* death number */
				}
				route = route->nextRouteLink;
			}
			net = net->hash_next;
		}
		UNLOCK(RIT.table[i].lock, pl);
	}
	RipLanDeathScream(rrLanKey);
	ageNets = TRUE;
	RipScheduleRip(DIRECT);

	NTR_VLEAVE();
	return;
}

/*
 * void RipRemoveNetRoute(netListEntry_t *net, netRouteEntry_t *route)
 *  Remove a route entry from a net entry.
 *
 * Calling/Exit State:
 *  BUCKET LOCK HELD AROUND CALL.
 *
 * Lock usage - Code/structures protected:
 *  RIT.table[n].lock
 */
void
RipRemoveNetRoute(netListEntry_t *net, netRouteEntry_t *route)
{	routePtrPtr_t		*lastRoute;
	netRouteEntry_t		*nextRoute;

 	NTR_ENTER(2, net, route, 0,0,0);

	lastRoute = (routePtrPtr_t *) &(net->routeListLink);
	while (lastRoute->routePtr != NULL) {
		if (lastRoute->routePtr == route) {
			lastRoute->routePtr = route->nextRouteLink;
			break;
		}
		nextRoute = lastRoute->routePtr;
		lastRoute = (routePtrPtr_t *) &(nextRoute->nextRouteLink);
	}
	nextRoute = net->routeListLink;
	if (nextRoute == NULL) {
		/* we lost our last router */
 		NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
 				"net 0x%X lost (last) route",
 				GETINT32(net->netIDNumber)));
		net->hopsToNet = MAX_MAX_HOPS;
		net->entryChanged = CHANGED;
	} else if ((net->hopsToNet != nextRoute->routeHops)
				|| (net->timeToNet != nextRoute->routeTime)) {
		/* the distance to the net has changed */
		net->hopsToNet = nextRoute->routeHops;
		net->timeToNet = nextRoute->routeTime;
		net->entryChanged = CHANGED;
	}
 	NTR_VLEAVE();
	return;
}

/*
 * void RipAddNetRoute(netListEntry_t *net, netRouteEntry_t *route)
 *  Link a route entry into a net's route list.
 *
 * Calling/Exit State:
 *  BUCKET LOCK HELD AROUND CALL
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipAddNetRoute(netListEntry_t *net, netRouteEntry_t *route)
{	routePtrPtr_t	*lastRoute;
	netRouteEntry_t	*nextRoute;

	NTR_ENTER(2, net, route, 0,0,0);

	lastRoute = (routePtrPtr_t *) &(net->routeListLink);
	nextRoute = net->routeListLink;
	while ((nextRoute != NULL)
			&& ((nextRoute->routeTime < route->routeTime)
				|| ((nextRoute->routeTime == route->routeTime)
					&& (nextRoute->routeHops < route->routeHops)))) {
		lastRoute = (routePtrPtr_t *) &(nextRoute->nextRouteLink);
		nextRoute = nextRoute->nextRouteLink;
	}
	route->nextRouteLink = nextRoute;
	lastRoute->routePtr = route;

	nextRoute = net->routeListLink;
	if ((net->hopsToNet != nextRoute->routeHops)
			|| (net->timeToNet != nextRoute->routeTime)) {
		/* the "distance" to this net has changed */
		net->hopsToNet = nextRoute->routeHops;
		net->timeToNet = nextRoute->routeTime;
		net->entryChanged = CHANGED;
		/* Now strip any WAN or STAR links that are not "best" */
		while (nextRoute != NULL) {
			if (((net->timeToNet != nextRoute->routeTime)
						|| (net->hopsToNet != nextRoute->routeHops))
					&& ((nextRoute->routeStatus
						& (NET_WAN_BIT | NET_STAR_BIT)) != 0)) {
				/* STAR or WAN info no longer best */
				RipRemoveNetRoute(net, nextRoute);
				nextRoute = net->routeListLink;
			} else {
				nextRoute = nextRoute->nextRouteLink;
			}
		}
	}
	NTR_VLEAVE();
	return;
}

/*
 * void RIPdigestRouterPacket(void *rrLanKey, mblk_t *mp)
 *  A RIP packet is being passed to us across the Replaceable
 *  Router Interface.  We are guaranteed a contiguous packet.
 *
 * Calling/Exit State:
 *  No locks held around call.
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RIPdigestRouterPacket(void *rrLanKey, mblk_t *mp)
{	uint16	operation;
	uint16	length;
	routePacket_t	*routerPacket;

	NTR_ENTER(2, rrLanKey, mp, 0,0,0);

	if (rrLanKey == NULL) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"RIPdigestRouterPacket: NULL rrLanKey"));
		NTR_VLEAVE();
		return;
	}

	ripStats.ReceivedPackets++;

	routerPacket = (routePacket_t *)mp->b_rptr;
	length = PGETINT16(&routerPacket->len);
	if (((int)length < MIN_ROUTE_PACKET)
			|| ((((int)length - ROUTE_PACKET_SIZE) % ROUTE_ENTRY_SIZE) != 0)) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"RIPdigestRouterPacket: Bad length router packet 0x%X",
				length));
		/*
		** Note: this packet has been identified as bad, however it
		** will still be passed upstream if the router socket is
		** open.  This will allow debugging of router code by an
		** application.  Caution should be taken when writing
		** router applications to make sure packets are valid.
		*/
		ripStats.ReceivedBadLength++;
		NTR_VLEAVE();
		return;
	}

	GETALIGN16(&routerPacket->operation, &operation);
	operation = GETINT16(operation);
	NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"RIPdigestRouterPacket: Router operation is %d",
			operation));
	switch(operation) {
	case ROUTE_REQUEST:
		ripStats.ReceivedRequestPackets++;
		if (routerType & CLIENT_ROUTER) {
			NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
				"RIPdigestRouterPacket: Silent RIP don't respond to request"));
			break;
		}
		RipGiveRouterInfo(rrLanKey, mp);
		break;
	case ROUTE_RESPONSE:
		ripStats.ReceivedResponsePackets++;
		RipAcceptRouterInfo(rrLanKey, routerPacket, length);
		break;
	default:
		ripStats.ReceivedUnknownRequest++;
		break;
	}
	NTR_VLEAVE();
	return;
}

/*
 * void RipGiveRouterInfo(void *rrLanKey, mblk_t *mp)
 *
 * We have received a request for route information.  If it is
 * a broadcast request, use Split-Horizon in forming a response.
 * If the request was targeted specifically to us, ignore split
 * horizon.  The request is composed of one or more Route Entries
 * (net, ticks, hops fields), which we respond to with completed
 * routing entries for the networks we know about.
 *
 * Calling/Exit State:
 *  No locks held around call.
 *
 * Lock usage - Code/structures protected:
 *  Bucket locked for each net requested
 */
void
RipGiveRouterInfo(void *rrLanKey, mblk_t *mp)
{	routePacket_t		*routeReq; /* incoming request */
	routePacket_t		*routeAns; /* outgoing reply */
	routeEntry_t		*request;
	routeEntry_t		*answer;
			int				i;
			int				skip;
			int				count;
			int				repCount;
			int				splitHorizon;
			pl_t			pl;
			uint16			localTime;
			uint16			pktLen;
			uint32			reqTargetNet;
			mblk_t			*newMp;
			netListEntry_t	*net;
			netRouteEntry_t	*route;

	NTR_ENTER(2, rrLanKey, mp, 0,0,0);

	routeReq = (routePacket_t *)mp->b_rptr;
	pktLen = PGETINT16(&routeReq->len);

	count = ((int)pktLen - ROUTE_PACKET_SIZE) / ROUTE_ENTRY_SIZE;
	if (count < 1) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipGiveRouterInfo: No Nets requested"));
		NTR_VLEAVE();
		return;
	}
	NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"RipGiveRouterInfo: for %d network(s)", count));

	request = routeReq->routeTable;
	for (i = 0; i < count; i++) {
		if (IPXCMPNET(&(request->targetNet), AllNets)) {
			RipSendRouterInfo(rrLanKey, routeReq->src.net,
								routeReq->src.node,
								routeReq->src.sock, FULL_INFO);
			NTR_VLEAVE();
			return;
		}
		request++;
	}
	request = routeReq->routeTable;

	/* If we are not answering a broadcast (ie, this is a
	** directed request), do not use split horizon (per spec).
	*/
	splitHorizon = IPXCMPNODE(AllHosts, routeReq->dest.node) ? -1 : 0;

	/* Since RIP pkts are only "on loan" to us, we can't use
	** the request pkt for the response, so alloc a response pkt
	*/
	if ((newMp = allocb(pktLen, BPRI_MED)) == NULL) {
		ripStats.SentAllocFailed++;
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipSendRouterInfo: allocb failed for rip reply, size 0x%X",
			pktLen));
		NTR_VLEAVE();
		return;
	}
	routeAns = (routePacket_t *)newMp->b_rptr;
	routeAns->dest = routeReq->src;
	routeAns->operation = GETINT16((uint16) ROUTE_RESPONSE);
	answer = routeAns->routeTable;

	localTime = RipLanTime(rrLanKey);
	repCount = 0;
	/*
	** Fill in the Router Packet entries
	*/
	for ( ; count > 0; --count, ++request) {
		IPXCOPYNET(&(request->targetNet), &reqTargetNet);
		pl = LOCK(RIT.table[RIP_NET_HASH(&reqTargetNet)].lock, plstr);
		net = RipGetNetEntry(reqTargetNet);
		if (net != NULL) {
			/* SMP bucket is locked */
 			if (net->hopsToNet < MaxHops) {
				skip = 0;
				if ((splitHorizon != 0)
						&& ((net->netStatus & NET_LOCAL_BIT) == 0)) {
					route = net->routeListLink;
					while ((route != NULL)
							&& (route->routeTime == net->timeToNet)){
						if (route->rrLanKey == rrLanKey) {
							/* Since we originally heard about this
							** net from someone else on this LAN,
							** they are the best source for this
							** info, and we should let them answer
							*/
							skip = -1;
							break; /* out of while (route && ...) */
						}
						route = route->nextRouteLink;
					}
				}
				if (!skip) {
					IPXCOPYNET(&request->targetNet, &answer->targetNet);
					answer->targetHops
						= GETINT16((uint16)(net->hopsToNet + 1));
					answer->targetTime
						= GETINT16(net->timeToNet + localTime);
					answer++;
					repCount++;
				}
			}
		}
		UNLOCK(RIT.table[RIP_NET_HASH(&reqTargetNet)].lock, pl);
	}
	if (repCount > 0) {
		pktLen = ROUTE_PACKET_SIZE + (repCount * ROUTE_ENTRY_SIZE);
		PPUTINT16(pktLen, &routeAns->len);
		newMp->b_wptr += pktLen;
		ripStats.SentResponsePackets++;
		RipDispatchPkt(rrLanKey, newMp, RR_NOPACE);
	} else {
		/* Discard packet that wound up not
		** getting any route info filled in.
		*/
		NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
			"RipGiveRouterInfo: freeing header-only router packet"));
		freemsg(newMp);
	}
	NTR_VLEAVE();
	return;
}

/*
 * void RipSendRouterInfo(void *rrLanKey, uint8 *destNet,
 *							uint8 *destNode, uint8 *destSock,
 *							int fullInfo)
 *
 * RipSendRouterInfo() sends router information out the specified
 * LAN destined for the indicated address.  Even if the "fullInfo"
 * flag is set, only the information NOT gleaned from routers on
 * the same network is broadcast! (Split horizon ignored on for
 * directed requests/responses.)
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipSendRouterInfo(void *rrLanKey, uint8 *destNet, uint8 *destNode,
					uint8 *destSock, int fullInfo)
{	int					splitHorizon;
	int					count, maxEntries;
	pl_t				pl;
	void				*ipxLanKey;
	uint8				node[6];
	uint8				flags;
	uint8				lanDeath;
	uint16				localTime;
	uint16				packetLength;
	uint32				i;
	uint32				hashIndex;
	uint32				nextNetId;
	mblk_t				*newMp;
	netListEntry_t		*net;
	netRouteEntry_t		*route;
	routeEntry_t		*entry;
	routePacket_t		*routeAns;

	NTR_ENTER(4, rrLanKey, destNet, destNode, fullInfo, 0);

	/* SMP if net->hopsToNet > MaxHops, rrLanKey
	** SMP should have been invalidated and we're not here
	*/
	if ((nextNetId = RIPLanData(rrLanKey, &ipxLanKey, node, NULL)) == 0) {
		NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"RipSendRouterInfo: Can't get data on rrLanKey 0x%X",
			rrLanKey));
		NTR_VLEAVE();
		return;
	}
	hashIndex = RIP_NET_HASH(&nextNetId);

	pl = LOCK(RIT.table[hashIndex].lock, plstr);
	if ((net = RipGetNetEntry(nextNetId)) == NULL) {
		NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"RipSendRouterInfo: No entry for lan net 0x%X",
			GETINT32(nextNetId)));
		UNLOCK(RIT.table[hashIndex].lock, pl);
		NTR_VLEAVE();
		return;
	}
	if (net->hopsToNet >= MaxHops)
		lanDeath = 1;
	else
		lanDeath = 0;
	/* mustAdvertise field is a waste.  Should ask "is Net local
	** && changed? if so, give full info
	*/
	if (fullInfo == NET_REFER)
		fullInfo = net->mustAdvertise;

	UNLOCK(RIT.table[hashIndex].lock, pl);

	localTime = RipLanTime(rrLanKey);
	packetLength = RipMaxLanPkt(rrLanKey);

	maxEntries = ((uint16)(packetLength - ROUTE_PACKET_SIZE)
							/ ROUTE_ENTRY_SIZE);
	if (IPXCMPNODE(AllHosts, destNode)) {
		splitHorizon = -1;
		flags = RR_BCAST;
	} else {
		splitHorizon = 0;
		flags = 0;
	}

	/* Don't pace lan death notifications - they have to get
	** out without hitting the lipmxlwsrv.  Otherwise, by the
	** time the service routine runs, qbot and rrLanKey are NULL
	*/
	if (lanDeath) {
		flags |= RR_QUEUE;
	} else {
		flags |= RR_PACE;
	}

	/* bypass empty hash buckets */
	for (i = 0; i < RIT.size; i++) {
		pl = LOCK(RIT.table[i].lock, plstr);
		net = RIT.table[i].list;
		if (net != NULL)
			break;	/* SMP with lock held */
		UNLOCK(RIT.table[i].lock, pl);
	}

	if (i >= RIT.size) { /* no networks? */
		/* SMP no lock to release */
		ripStats.SentBadDestination++;
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipSendRouterInfo: No nets in RIT"));
		NTR_VLEAVE();
		return;
	}
	nextNetId = RIT.table[i].list->netIDNumber;
	UNLOCK(RIT.table[i].lock, pl);

Restart:
	NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipSendRouterInfo: Alloc pktLen = %d, maxEntries = %d",
			packetLength, maxEntries));
	if ((newMp = allocb(packetLength, BPRI_MED)) == NULL) {
		ripStats.SentAllocFailed++;
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipSendRouterInfo: allocb failed for rip reply, size 0x%X",
			packetLength));
		NTR_VLEAVE();
		return;
	}
	/* fill in minimal header info */
	routeAns = (routePacket_t *)newMp->b_rptr;
	IPXCOPYNET(destNet, routeAns->dest.net);
	IPXCOPYNODE(destNode, routeAns->dest.node);
	if (destSock)
		IPXCOPYSOCK(destSock, routeAns->dest.sock);
	else
		IPXCOPYSOCK(&RipSocket, routeAns->dest.sock);
	routeAns->operation = GETINT16(ROUTE_RESPONSE);

	entry = routeAns->routeTable;
	count = 0;

	i = RIP_NET_HASH(&nextNetId);
	pl = LOCK(RIT.table[i].lock, plstr);
	net = RIT.table[i].list;
	while ((net) && (net->netIDNumber != nextNetId))
		net = net->hash_next;

	if (!net) {
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RipSendRouterInfo: entry for net 0x%X disappeared!",
			GETINT32(nextNetId)));
		UNLOCK(RIT.table[i].lock, pl);
		ripStats.SentBadDestination++;
		freemsg(newMp);
		NTR_VLEAVE();
		return;
	}
	NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
		"RipSendRouterInfo: header completed"));

	do {
		while (net != NULL) {
			if (fullInfo || (net->entryChanged == CHANGED)) {
				if (count >= maxEntries) {
					nextNetId = net->netIDNumber;
					UNLOCK(RIT.table[i].lock, pl);
					PPUTINT16(packetLength, &routeAns->len);
					newMp->b_wptr += packetLength;
					NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
						"RipSendRouterInfo: sending full router pkt"));
					ripStats.SentResponsePackets++;
					RipDispatchPkt(rrLanKey, newMp, flags);
					goto Restart;
				}
				/*
				** Now check to see if I should report this net on this
				** connected LAN, or is my router on this LAN?
				*/
				route = net->routeListLink;

				if (splitHorizon)
					while ((route != NULL)
							 && (route->routeTime == net->timeToNet)) {
						if (route->rrLanKey == rrLanKey)
							goto GetNextEntry;
						route = route->nextRouteLink;
					}

				/*
				** Avoid unintentional broadcast
				** storm when a network goes down.
				*/
				if (net->hopsToNet == (MaxHops - 1))
					goto GetNextEntry;

				IPXCOPYNET(&net->netIDNumber, &entry->targetNet);
				if ((net->hopsToNet >= MaxHops) || lanDeath)
					entry->targetHops = GETINT16(MaxHops);
				else
					entry->targetHops
						= GETINT16((uint16)(net->hopsToNet + 1));
				entry->targetTime
					= GETINT16(net->timeToNet + localTime);
				entry++;
				count++;
			}
		GetNextEntry:
			net = net->hash_next;
		}
		UNLOCK(RIT.table[i].lock, pl);
		i++;
		if (i < RIT.size) {
			pl = LOCK(RIT.table[i].lock, plstr);
			net = RIT.table[i].list;
		}
	} while (i < RIT.size);

	if (count > 0) {
		packetLength = ROUTE_PACKET_SIZE + (count * ROUTE_ENTRY_SIZE);
		PPUTINT16(packetLength, &routeAns->len);
		newMp->b_wptr += packetLength;
		NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
			"RipSendRouterInfo: sending router pkt with 0x%X entries",
			count));
		ripStats.SentResponsePackets++;
		RipDispatchPkt(rrLanKey, newMp, flags);
	} else {
		/* Discard packet that wound up not
		** getting any route info filled in.
		*/
		NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
			"RipSendRouterInfo: freeing header-only router packet"));
		freemsg(newMp);
	}
	NTR_VLEAVE();
	return;
}

/*
 * void * RipRequestRouteInfo(void *notused, void *rrLanKey)
 *
 * Broadcast a Request Packet on a given LAN
 * soliciting information about all nets.
 *
 * Calling/Exit State:
 *  No locks held around call.
 *
 * Lock usage - Code/structures protected:
 *  NONE - just send out a RIP request without referencing RIT
 */
/*ARGSUSED*/
void *
RipRequestRouteInfo(void *notused, void *rrLanKey)
{	mblk_t				*reqMp;
	routePacket_t		*reqRouterPacket;

	NTR_ENTER(1, rrLanKey, 0,0,0,0);

	if ((reqMp = allocb(MIN_ROUTE_PACKET, BPRI_MED)) == NULL) {
		ripStats.SentAllocFailed++;
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipRequestRouteInfo: allocb failed size = 0x%X",
				MIN_ROUTE_PACKET));
		return((void *)NTR_LEAVE(0));
	}
	reqRouterPacket = (routePacket_t *)reqMp->b_rptr;
	reqMp->b_wptr += MIN_ROUTE_PACKET;
	PPUTINT16(MIN_ROUTE_PACKET, &reqRouterPacket->len);
	/* net will always be local, filled in by RipDispatchPkt() */
	IPXCOPYNODE(AllHosts, reqRouterPacket->dest.node);
	IPXCOPYSOCK(&RipSocket, reqRouterPacket->dest.sock);

	/* src NODE filled out in RIPLanData() */
	reqRouterPacket->operation = GETINT16(ROUTE_REQUEST);
	reqRouterPacket->routeTable[0].targetNet = (uint32)ALL_LANS;
	reqRouterPacket->routeTable[0].targetHops = 0;
	reqRouterPacket->routeTable[0].targetTime = 0;
	RipDispatchPkt(rrLanKey, reqMp, RR_NOPACE);
	ripStats.SentRequestPackets++;
	return((void *)NTR_LEAVE(0));
}

/*
 * void RipAgeRouters(long now)
 *
 * Called only from ripxchain
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipAgeRouters(long now)
{	int					changed, timer;
	pl_t				pl;
	uint32				i, deadNet;
	struct xx {
		uint32	net;
		struct xx	*next;
	} *deadList, *tmpDead;
	netListEntry_t		*nextNet;
	netRouteEntry_t		*nextRoute;
	netListEntry_t		*net;
	netRouteEntry_t		*route;

	NTR_ENTER(1, now, 0	,0,0,0);

	ageNets = FALSE;
	changed = FALSE;
	for (i = 0; i < RIT.size; i++) {
 		pl = LOCK(RIT.table[i].lock, plstr);
		net = RIT.table[i].list;
		while (net != NULL) {
			/*
			** Age this network information
			*/
			route = net->routeListLink;
			while (route != NULL) {
				if (now && ((route->routeStatus & (NET_LOCAL_BIT |
						NET_RELIABLE_BIT)) == 0)) {
					timer = (uint32)route->timer - BasisMultiplier;
					if (timer > 0)
						route->timer = (uint16)timer;
					else {
 						NWSLOG((RIPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
 							"setting net 0x%X timer to ageOut (net/route flags = 0x%X 0x%X)",
 							GETINT32(net->netIDNumber), net->netStatus,
 							route->routeStatus));
						route->timer = 0;
					}
				}

				/*
				** Age information not directly connected or slow
				*/
				if (route->timer == 0) { /* magic number */
 					NWSLOG((RIPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
 						"aging out a route to net 0x%X, flags = 0x%X",
 						GETINT32(net->netIDNumber), net->netStatus));
					nextRoute = route->nextRouteLink;
					RipRemoveNetRoute(net, route);
					NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
						"RipAgeRouters: KMEM: Free route structure at 0x%X, size %d",
							route, sizeof(netRouteEntry_t)));
					ripx_free(route, sizeof(netRouteEntry_t));
					changed = TRUE;
					route = nextRoute;
				} else
					route = route->nextRouteLink;
			}
			net = net->hash_next;
		}
		UNLOCK(RIT.table[i].lock, pl);
	}

	/*
	** Send updates to everyone
	*/
	RIPSendUpdates(changed, now);

	/*
	** Changed information has been sent - update entries
	** Clean out any "unreachable" nets
	*/
	deadList = NULL;
	for (i = 0; i < RIT.size; i++) {
		pl = LOCK(RIT.table[i].lock, plstr);
		net = RIT.table[i].list;
		while (net != NULL) {
			net->entryChanged = 0;
			if (net->hopsToNet >= MaxHops) {
 				NWSLOG((RIPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
 					"RipAgeRouters: net 0x%X: hops %d >= MaxHops (net flags = 0x%X)",
 						GETINT32(net->netIDNumber), net->hopsToNet,
 						net->netStatus));
				nextNet = net->hash_next;
				if ((deadNet = RipKillNetEntry(net, &RIT)) != 0) {
					if ((tmpDead
							= (struct xx *)ripx_alloc(sizeof(struct xx),
														KM_NOSLEEP))
								!= NULL) {
								NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
									"RipAgeRouters: KMEM: Allocate xx structure at 0x%X, size %d", 
										tmpDead, sizeof(struct xx)));
						tmpDead->net = deadNet;
						tmpDead->next = deadList;
						deadList = tmpDead;
					}
				}
			} else {
				nextNet = net->hash_next;
			}
			net = nextNet;
		}
		UNLOCK(RIT.table[i].lock, pl);
	}
	while ((tmpDead = deadList) != NULL) {
		deadList = tmpDead->next;
		tmpDead->next = NULL;
		NotifySapNetDead(tmpDead->net);
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipAgeRouters: KMEM: Free xx structure at 0x%X, size %d", 
				tmpDead, sizeof(struct xx)));
		ripx_free(tmpDead, sizeof(struct xx));
	}
	NTR_VLEAVE();
	return;
}

/*
 * uint32 RipKillNetEntry(netListEntry_t *net)
 *
 * A network entry in the RIT has been deemed worthy of death.
 * Remove it from the RIT.
 * THIS IS THE ONLY PLACE THAT A NETWORK ENTRY WILL BE REMOVED,
 * AND IT WILL ALWAYS HAPPEN IN SERVICE ROUTINE CONTEXT (ie; from
 * ripchain->RipAgeRouters).
 *
 * Calling/Exit State:
 *  BUCKET LOCK HELD AROUND CALL.
 *
 * Lock usage - Code/structures protected:
 *
 */
uint32
RipKillNetEntry(netListEntry_t *net, RtInfoTable_t *rit)
{	uint32	hashIndex;
	netListEntry_t		*tNet;
	uint32					dNet;
	netRouteEntry_t			*route;
	netRouteEntry_t			*nextRoute;

	NTR_ENTER(2, net, rit,0,0,0);

	if (net == NULL) {
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RipKillNetEntry: netp 0x%X invalid", net));
		return((uint32)NTR_LEAVE(0));
	}

	/*
	** Remove the network from the ipxrLocalNet table
	** and the net linked list and free their memory blocks.
	*/
	dNet = net->netIDNumber;
	NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"RipKillNetEntry: Killing Net Number 0x%X",
			GETINT32(dNet)));
	if ((net->netStatus & NET_LOCAL_BIT) != 0) {
#ifdef HARD_DEBUG
		/*
		 *+ Debug message
		 */
		cmn_err(CE_WARN, "RipKillNetEntry: killing local net 0x%X",
				GETINT32(dNet));
#endif /* def DEBUG */
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RipKillNetEntry: Killing local net 0x%X",
			GETINT32(dNet)));
	}

	hashIndex = RIP_NET_HASH(&dNet);
	tNet = rit->table[hashIndex].list;

	while ((tNet != NULL) && (tNet->netIDNumber != dNet))
		tNet = tNet->hash_next;

	if (tNet != NULL) {
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RipKillNetEntry: net %x ind %d next %x ",
			GETINT32(dNet), hashIndex, tNet->hash_next));
		if (tNet->hash_prev) {
			tNet->hash_prev->hash_next = tNet->hash_next;
			if (tNet->hash_next)
				tNet->hash_next->hash_prev = tNet->hash_prev;
		} else {
			rit->table[hashIndex].list = tNet->hash_next;
			if (tNet->hash_next) tNet->hash_next->hash_prev = NULL;
		}
	} else {
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RipKillNetEntry: unknown net 0x%X ", GETINT32(dNet)));
		dNet = 0;
	}

	route = net->routeListLink;
	while (route != NULL) {
		nextRoute = route->nextRouteLink;
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipKillNetEntry: KMEM: Free route structure at 0x%X, size %d",
				route, sizeof(netRouteEntry_t)));
		ripx_free(route, sizeof(netRouteEntry_t));
		route = nextRoute;
	}
	NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
		"RipKillNetEntry: KMEM: Free net structure at 0x%X, size %d",
			net, sizeof(netListEntry_t)));
	ripx_free(net, sizeof(netListEntry_t));
	return((uint32)NTR_LEAVE(dNet));
}

/*
 * void RipAdvertise(void)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipAdvertise(void)
{
	NTR_ENTER(0, 0,0,0,0,0);

	foundNets = FALSE;

	if (routerType & CLIENT_ROUTER) {
		NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"RipAdvertise: Silent RIP don't Advertise"));
		NTR_VLEAVE();
		return;
	}

	/*
	** First tell the world about myself.
	*/
	(void)RRIPX.UseMethodWithLanKey(ALL_LANS, (void *)ADVERTISE,
									RIPSendRouterInfoSWITCH);

	/*
	** Politely ask on all LANs about the rest of the world's routers.
	*/
	(void)RRIPX.UseMethodWithLanKey(ALL_LANS, NULL,
									RipRequestRouteInfo);

	RIPClearChanged();
	NTR_VLEAVE();
	return;
}

/*
 * void RipPostLostRoutes(void)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipPostLostRoutes(void)
{	uint32			i;
	netListEntry_t	*net;
	netListEntry_t	*tnet;
	pl_t	pl;

	NTR_ENTER(0, 0,0,0,0,0);

	lostNets = FALSE;

	for (i = 0; i < RIT.size; i++) {
		pl = LOCK(RIT.table[i].lock, plstr);
		net = RIT.table[i].list;
		while (net != NULL) {
			if (net->routerLostNetFlag) {
				net->routerLostNetFlag = 0;
				if (net->hopsToNet < MaxHops)
					net->entryChanged = CHANGED;
			}
			net = net->hash_next;
		}
		UNLOCK(RIT.table[i].lock, pl);
	}

	(void)RRIPX.UseMethodWithLanKey(ALL_LANS, (void *)POST_LOST,
									RIPSendRouterInfoSWITCH);

	for (i = 0; i < RIT.size; i++) {
		pl = LOCK(RIT.table[i].lock, plstr);
		net = RIT.table[i].list;
		while (net != NULL) {
			net->entryChanged = 0;
			tnet = net;
			net = net->hash_next;
			if (tnet->hopsToNet >= MaxHops) {
				NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
					"RipPostLostRoutes: Lost net 0x%X advertised, Remove it",
 					GETINT32(tnet->netIDNumber)));
				RipKillNetEntry(tnet, &RIT);
			}
		}
		UNLOCK(RIT.table[i].lock, pl);
	}
	NTR_VLEAVE();
	return;
}

/*
 * int ripxchain(queue_t *q)
 *
 * ripxchain() takes the place of ipxruwsrv(). No messages
 * are de-queued - it just starts and maintains the chain
 * of RIP broadcasts.
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
/*ARGSUSED*/
int
ripxchain(queue_t *q)
{	long	now;
	/*
	** The ripxchain() service procedure is used by the router
	** to send periodic updates to all known routers and query them
	** for information.  It is scheduled by a timeout routine and/or
	** events that wish to cause a premature wakeup of the router.
	**
	 * NOTE: This service procedure never takes messages off of its
	 * message queue.  No routine should put messages on its message
	 * queue.  It is in no way connected with the normal flow of data
	 * packets into or out of the machine.
	 */

	/* Router Maintenance routine */
	NTR_ENTER(1, q, 0,0,0,0);

	/*
	** If this routine is called inadvertently by flow control or
	** some other method, simply return.  Only execute this code if
	** the RIP flag is set.
	*/
	if (ripFlag == FALSE) {
		return(NTR_LEAVE(0));
	}
	/*
	** advertise lost nets before aging
	*/
	if (lostNets) {
		RipPostLostRoutes();
	}
	drv_getparm(LBOLT, (void *)&now);
	if ((now - lastRouterAgedTime) >= UpdateTicks) {
		NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
			"ripxchain: now = 0x%X, prev = 0x%X, diff = 0x%X",
			now, lastRouterAgedTime, now - lastRouterAgedTime));
#ifdef HARDDEBUG
		/*
		 *+ DEBUG message
		 */
		cmn_err(CE_NOTE,"chain : now = 0x%X, last = 0x%X, diff = 0x%X, update in 0x%X ticks\n",
			now, lastRouterAgedTime, now - lastRouterAgedTime, UpdateTicks);
#endif /* def DEBUG */
		RipAgeRouters(now);
		lastRouterAgedTime = now;
	}

TryItAllAgain:
	if (foundNets) {
		RipAdvertise();
		goto TryItAllAgain;
	}

	if (lostNets) {
		RipPostLostRoutes();
		goto TryItAllAgain;
	}

	if (ageNets) {
		RipAgeRouters(0);
		goto TryItAllAgain;
	}

	ripFlag = FALSE;

	return(NTR_LEAVE(0));
}

/*
 * void RipScheduleRip(int call_type)
 *	call_type is NULL if system time_out, 1 if direct call 
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipScheduleRip(void *call_type)
{
	NTR_ENTER(1, call_type, 0,0,0,0);

	if ( call_type == SYSTEM) {
		if( ripControlClosing) {
			ripId = 0;
		} else {
			ripId = itimeout(RipScheduleRip, SYSTEM, UpdateTicks, pltimeout);
		}
	}
	if (RipQ) {
		ripFlag = TRUE;
		qenable(RipQ);
	}
	NTR_VLEAVE();
	return;
}

/*
 * void RipUntimeout(void)
 *	untimeout any/all timers running we are closing down, called from ripxclose.
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipUntimeout(void )
{
	int	count;
	
	NTR_ENTER(0, 0, 0,0,0,0);
	count = 10;
	while (count) {
		count--;
		if (ripId) {
			untimeout(ripId);
			ripId = 0;
			count = 10;
		}
		delay(1);
	}

	NTR_VLEAVE();
	return;
}

/*
 * void RipClearRIT(RtInfoTable_t *, int)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipClearRIT(RtInfoTable_t *tbl, int clearLocals)
{	netListEntry_t	*np;
	pl_t	pl;
	int	i;

	NTR_ENTER(2, tbl, clearLocals, 0,0,0);

	for (i = 0; i < tbl->size; i++) {
		pl = LOCK(tbl->table[i].lock, plstr);
		for (np = tbl->table[i].list; np; np = np->hash_next) {
			if (clearLocals || (!(np->netStatus & NET_LOCAL_BIT)))
				RipKillNetEntry(np, tbl);
			else
				np->mustAdvertise = -1;
		}
		UNLOCK(tbl->table[i].lock, pl);
	}

	NTR_VLEAVE();
	return;
}

/*
 * void RipKillTableAndRips(RtInfoTable_t *rit)
 *  Called from RIPfinish().  The control device is closing
 *  and it is time to shut down all activity.  The router has
 *  been deregistered, and all rrLanKeys held by lipmx have
 *  been submitted for deregistration (though their storage may
 *  not have been freed yet).
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipKillTableAndRips(RtInfoTable_t *rit)
{
	NTR_ENTER(0, 0,0,0,0,0);

	RipQ = NULL;
	RipUntimeout();
	RipClearRIT( rit, KILL_LOCALS );
	NTR_VLEAVE();
	return;
}

/*
 * void RipResetRouter(void)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipResetRouter(void)
{
	NTR_ENTER(0, 0,0,0,0,0);

	RipClearRIT( &RIT, SAVE_LOCALS );
	if (routerType & CLIENT_ROUTER) {
		/*
		** Politely ask on all LANs about the rest of the world's routers.
		*/
		(void)RRIPX.UseMethodWithLanKey(ALL_LANS, NULL, RipRequestRouteInfo);
	} else {
		foundNets = TRUE;
		RipScheduleRip(DIRECT);
	}

	NTR_VLEAVE();
	return;
}

/*
 * void RIPfinish(void)
 *
 * Control device is closing
 * Stop doing things
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RIPfinish(void)
{	RtInfoTable_t	tmpTbl;
	int	i;

	NTR_ENTER(0, 0,0,0,0,0);

	/*
	 * We have deregistered the router, so no new calls to rip can
	 * come in. Wait for all inprogress calls to RIP to exit before
	 * changing the RIT table
	 */
#ifdef NW_BCI
    while(ATOMIC_INT_READ(&RRIPX.Ripcount)) {
        delay(1);
    }
#else
    while(ATOMIC_INT_READ(&RRIPX.Ripcount)) {
        drv_usecwait(10);
    }
#endif

	tmpTbl = RIT;
	RIT.size = 0;
	RIT = DefaultRIT;
	RipKillTableAndRips(&tmpTbl);

	BasisMultiplier = 1;
	UpdateTicks = 30 * HZ;

	for (i = 0; i < tmpTbl.size; i++) {
		LOCK_DEALLOC(tmpTbl.table[i].lock);
		tmpTbl.table[i].lock = NULL;
	}
	if (tmpTbl.size) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPfinish: KMEM: Free hash structure at 0x%X, size %d", 
				tmpTbl.table, tmpTbl.size * sizeof(NetHash_t)));
		kmem_free(tmpTbl.table, tmpTbl.size * sizeof(NetHash_t));
	}

	NTR_VLEAVE();
	return;
}

/*
 * int RIPstart(void )
 *
 * Called in response to a RIPX_START_ROUTER/M_IOCTL.
 * This start the router after everthing is initialized.
 * Advertise all local nets, and ask about the world.
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RIPstart(queue_t *ripQ)
{
	NTR_ENTER(1, ripQ, 0, 0,0,0);

	RipQ = ripQ;			/* flag to start everything */
	if (routerType & CLIENT_ROUTER) {
		/*
		** Politely ask on all LANs about the rest of the world's routers.
		*/
		(void)RRIPX.UseMethodWithLanKey(ALL_LANS, NULL, RipRequestRouteInfo);
	}

	RipScheduleRip(SYSTEM);

	NTR_VLEAVE();
	return;
}
/*
 * int RIPInitialize(queue_t *ripQ, Initialize_t *initTable)
 *
 * Called in response to a RIPX_INITIALIZE/M_IOCTL.
 * This initializes everything.
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
int
RIPInitialize(Initialize_t *initTable)
{
	NTR_ENTER(1, initTable, 0,0,0,0);

	if (!RipSetMaxHops((uint16)initTable->hops))
		return(NTR_LEAVE(0));
	initTable->hops = (int8)MaxHops;

	routerType = initTable->type;
	NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"RIPInitialize: routerType set to %d", routerType));

	/* The comments describing this IOCTL say
	** the actual hash size will be returned.
	*/
	if ((initTable->size = (uint32)RIPSetRITHashSize(initTable->size)) == 0)
		return(NTR_LEAVE(0));

	RipRegisterRouter();

	return(NTR_LEAVE( 1 ));
}

/*
 * uint16 RipSetMaxHops(uint16 maxHops)
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
uint16
RipSetMaxHops(uint16 maxHops)
{
	NTR_ENTER(1, maxHops, 0,0,0,0);

	if (maxHops < 2) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipSetMaxHops: attempt to set MaxHops %d < 2", maxHops));
		return(NTR_LEAVE(0));
	}
	MaxHops = maxHops;

	NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
			"RipSetMaxHops: MaxHops set to %d", MaxHops));
	return(NTR_LEAVE( MaxHops ));
}

/*
 * uint16 RoundToPowerOf2(uint16 m)
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
uint16
RoundToPowerOf2(uint16 m)
{	unsigned	i;

	NTR_ENTER(1, m, 0,0,0,0);

	for (i = 0; i < 16; i++)
		if (m <= (uint16)(1 << i))
			break;
	if (i >= 16)
		i = 15;
	m = 1 << i;
	return(NTR_LEAVE(m));
}

/*
 * int RIPSetRITHashSize(uint16 hashSize)
 *  Called once and only once - during initialization.
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
int
RIPSetRITHashSize(uint16 hashSize)
{	uint16	i, j;
	NetHash_t  *hashTable;

	NTR_ENTER(1, hashSize, 0,0,0,0);

	if (RIT.size) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPSetRITHashSize: hash size already set to %d",
			RIT.size));
		return(NTR_LEAVE(0));
	}
	hashSize = RoundToPowerOf2(hashSize);
	if ((hashTable = (NetHash_t *)
				kmem_alloc((int)hashSize * sizeof(NetHash_t),
							KM_NOSLEEP))
			== NULL) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPSetRITHashSize: alloc failure on new hash size 0x%X",
			hashSize));
		return(NTR_LEAVE(0));
	}
	NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
		"RIPSetRITHashSize: KMEM: Alloc Hash Table at 0x%X, size %d", 
			hashTable, (int)hashSize * sizeof(NetHash_t)));
	for (i = 0; i < hashSize; i++) {
		hashTable[i].list = NULL;
		if ((hashTable[i].lock
				= LOCK_ALLOC(HASH_LOCK, plstr, &RipHashLkInfo,
								KM_NOSLEEP)) == NULL) {
			for (j = 0; j < i; j++) {
				LOCK_DEALLOC(hashTable[j].lock);
				hashTable[j].lock = NULL;
			}
			NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"RIPSetRITHashSize: KMEM: Free hash table at 0x%X, size %d", 
					hashTable, (int)hashSize * sizeof(NetHash_t)));
			kmem_free(hashTable, (int)hashSize * sizeof(NetHash_t));
			NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"RIPSetRITHashSize: hash size already set to %d",
				RIT.size));
			return(NTR_LEAVE(0));
		}
	}

	RipInitRouter(); /* just sets ripFlag to FALSE */

	RIT.mask = hashSize - 1;
	RIT.table = hashTable;

	RIT.stats.keys = hashSize;
	RIT.stats.accesses = 0;
	RIT.stats.collisions = 0;
	RIT.stats.failedFinds = 0;

	/* atomic assign, RIT ready to go */
	RIT.size = hashSize;

	return(NTR_LEAVE(hashSize));
}

/*
 * int RIPgetRouterTable(queue_t *q)
 *
 * RIPIGetRouterTable forms ROUTE_TABLE_SIZE message blocks
 * and sends them upstream until the entire router table
 * has been sent upstream to the calling process.  It then
 * returns from the ioctl with no errors.
 * If a failure occurs, the routine returns a -1 and the
 * ioctl fails.
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
int
RIPgetRouterTable(queue_t *q)
{	netListEntry_t	*net;
	netRouteEntry_t *route;
	routeInfo_t		*routeInfo;
				int				cCode;
				pl_t			pl;
				uint16			hops;
				uint16			i;
				uint32			nroutes = 0;
				uint32			netID;
				mblk_t			*tblMp, *outList, *tMp;

	NTR_ENTER(1, q, 0,0,0,0);

	if ((tblMp = allocb(ROUTE_TABLE_SIZE, BPRI_MED)) == NULL) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPgetRouterTable: allocb failed, size = 0x%X",
			ROUTE_TABLE_SIZE));
		return(NTR_LEAVE(0));
	}
	routeInfo = (routeInfo_t *)tblMp->b_wptr; /* Be Quiet Lint! */

	outList = NULL;
	for (i = 0; i < RIT.size; i++) {
		pl = LOCK(RIT.table[i].lock, plstr);
		net = RIT.table[i].list;
		while (net != NULL) {
			netID = GETINT32(net->netIDNumber);
			route = net->routeListLink;
			while (route != NULL) {
				if (DATA_REMAINING(tblMp) < ROUTE_INFO_SIZE) {
					if ((tMp = allocb(ROUTE_TABLE_SIZE, BPRI_MED))
							== NULL) {
						NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
							"RIPgetRouterTable: allocb failed, size = 0x%X",
							ROUTE_TABLE_SIZE));
						UNLOCK(RIT.table[i].lock, pl);
						/*
						**	End previous buffer, can't do any more
						**	Note: lint complains about routeInfo used
						**	before set, but trust me, it is set.
						*/
						cCode = 0;
						goto Leave;
					}
					tblMp->b_next = outList;
					outList = tblMp;
					tblMp = tMp;
				}
				routeInfo = (routeInfo_t *)tblMp->b_wptr;
				routeInfo->net = netID;
				RIPmapRRKeyToIpxKey(route->rrLanKey,
									(void *)&routeInfo->connectedLan);
				hops = (uint16)route->routeHops;
				routeInfo->hops = hops;
				routeInfo->time = route->routeTime;
				IPXCOPYNODE(route->forwardingRouter, routeInfo->node);
				routeInfo->endOfTable = (uint16)FALSE;
				nroutes++;
				/*
				**	Get next entry
				*/
				tblMp->b_wptr += ROUTE_INFO_SIZE;
				route = route->nextRouteLink;
			}
			net = net->hash_next;
		}
		UNLOCK(RIT.table[i].lock, pl);
	}
	cCode = 1;

Leave:
	routeInfo->endOfTable = (uint16)TRUE;
	while(outList) {
		tMp = outList;
		outList = tMp->b_next;
		tMp->b_next = NULL;
		qreply(q, tMp);
	}
	qreply(q, tblMp);
	NWSLOG((RIPXID,0,PNW_DATA_ASCII,SL_TRACE,
		"RIPgetRouterTable: found %d routes", nroutes));
	return(NTR_LEAVE( cCode ));
}

/*
 * void RipHashStats(hashStats_t *rtnStats)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 */
void
RipHashStats(hashStats_t *rtnStats)
{	uint32	i, chainIndex;
	pl_t	pl;
	netListEntry_t	*net;

	NTR_ENTER(1, rtnStats, 0,0,0,0);

	for (i = 0; i < RIT.size; i++) {
		pl = LOCK(RIT.table[i].lock, plstr);
		net = RIT.table[i].list;
		chainIndex = 0;

		while (net != NULL) {
			++RIT.stats.links;
			RIT.stats.posXcountSum += ++chainIndex;
			net = net->hash_next;
		}
		if (!chainIndex)
			++RIT.stats.blankKeys;
		UNLOCK(RIT.table[i].lock, pl);
	}
	*rtnStats = RIT.stats;
	NTR_VLEAVE();
	return;
}

/*
 * int RipHashBucketCounts(int counts[], uint16 buckets)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
int
RipHashBucketCounts(int counts[], uint16 buckets)
{	int		co;
	pl_t	pl;
	uint16	i;
	netListEntry_t *np;

	NTR_ENTER(2, counts, buckets, 0,0,0);

	if (buckets < RIT.size) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipDumpHashTable: array (%d) != buckets (%d)",
			buckets, RIT.size));
		return(NTR_LEAVE(-1));
	}

	for (i = 0; i < RIT.size; i++) {
		pl = LOCK(RIT.table[i].lock, plstr);
		co = 0;
		for (np = RIT.table[i].list; np; np = np->hash_next)
			co++;
		counts[i] = co;
		UNLOCK(RIT.table[i].lock, pl);
	}

	return(NTR_LEAVE(0));
}

/*
 * int RIPmapNetToIpxLanKey(uint32 targetNet, void **ipxLanKeyPtr)
 *
 * We know about Ipx through the ipxLanKey it gave us when
 * it registered it rrLanKey.  Ipx now wants us to examine
 * our Routing Information Table (RIT) and let it know which
 * lan (as represented by an ipxLanKey) a pkt to a target net
 * would be routed.
 * SIDE EFFECT: *ipxLanKeyPtr gets changed, indicates success of call !
 * This function gets light traffic
 *
 * Calling/Exit State:
 * SMP No lock held on entry
 *
 * Lock usage - Code/structures protected:
 * SMP Bucket lock held around call to RIPmakeDlHdr
 */
int
RIPmapNetToIpxLanKey(uint32 targetNet, void **ipxLanKeyPtr)
{	netListEntry_t	*net;
	netRouteEntry_t	*route;
	pl_t	pl;

	NTR_ENTER(2, GETINT32(targetNet), ipxLanKeyPtr, 0,0,0);

	/* SMP lock hash bucket */
	pl = LOCK(RIT.table[RIP_NET_HASH(&targetNet)].lock, plstr);
	net = RIT.table[(RIP_NET_HASH(&targetNet))].list;

	++RIT.stats.accesses;
	while ((net != NULL) && (net->netIDNumber != targetNet)) {
		++RIT.stats.collisions;
		net = net->hash_next;
	}

	if ((net == NULL) || ((route = net->routeListLink) == NULL)) {
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RIPgetLanToNet: didn't find route to net 0x%X",
			GETINT32(targetNet)));
		UNLOCK(RIT.table[RIP_NET_HASH(&targetNet)].lock, pl);
		++RIT.stats.failedFinds;
		*ipxLanKeyPtr  = (void *)-1;
		/* SMP unlock hash bucket */
		return((int)NTR_LEAVE(0));
	}
	UNLOCK(RIT.table[RIP_NET_HASH(&targetNet)].lock, pl);
	return((int)NTR_LEAVE(RIPmapRRKeyToIpxKey(route->rrLanKey,
											ipxLanKeyPtr)));
}

/*
 * mblk_t * RIPgetRouteInfo(ipxHdr_t *hdr, void **ipxLanKey)
 *
 * Ipx needs to route a packet.  We will examine the RIT to
 * see if we know a route to the destination network.  If we
 * do, then we point the ipxLanKey** to the key that Ipx uses
 * to represent the lan to us.  If that lan has a preHdr associated
 * with it, we fill in the node address and pass the mblk_t* back.
 * SIDE EFFECT: *ipxLanKey gets changed, indicates success of call !
 * This function gets *very* heavy traffic
 *
 * Calling/Exit State:
 * SMP No lock held on entry
 *
 * Lock usage - Code/structures protected:
 * SMP Bucket lock held around call to RIPmakeDlHdr
 *
 */
mblk_t *
RIPgetRouteInfo(ipxHdr_t *hdr, void **ipxLanKey)
{	mblk_t	*preHdr;
	pl_t	pl;
	uint32	targetNet;
	netListEntry_t	*net;
	netRouteEntry_t	*route;

	NTR_ENTER(2, hdr, ipxLanKey, 0,0,0);

	IPXCOPYNET(hdr->dest.net, &targetNet);
	pl = LOCK(RIT.table[RIP_NET_HASH(&targetNet)].lock, plstr);
	net = RIT.table[(RIP_NET_HASH(&targetNet))].list;

	++RIT.stats.accesses;
	while ((net != NULL) && (net->netIDNumber != targetNet)) {
		++RIT.stats.collisions;
		net = net->hash_next;
	}

	if ((net == NULL) || ((route = net->routeListLink) == NULL)) {
		++RIT.stats.failedFinds;
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RIPgetLanToNet: didn't find route to net 0x%X",
			GETINT32(targetNet)));
		*ipxLanKey  = (void *)-1;
		preHdr = NULL;
	} else {
		/* this call will change ipxLanKey */
		preHdr = RIPmakeDlHdr(hdr, ipxLanKey, route);
	}
	UNLOCK(RIT.table[RIP_NET_HASH(&targetNet)].lock, pl);
	return((mblk_t *)NTR_LEAVE(preHdr));
}

/*
 * void RIPcheckSapSource(checkSapSource_t *sapSource)
 *
 * Check a sap source for validity
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RIPcheckSapSource(checkSapSource_t *sapSource)
{	netListEntry_t *srvrNet, *lNet;
	netRouteEntry_t *srvrRoute;
	uint32 networkOrder;
	uint32 routeLan;
	pl_t	pl;

	NTR_ENTER(1, sapSource, 0,0,0,0);

	/* assume good til proven otherwise */
	sapSource->result = 0;

	networkOrder = RRIPX.MapSapLanToNetwork(sapSource->connectedLan);
	pl = LOCK(RIT.table[RIP_NET_HASH(&networkOrder)].lock, plstr);
	lNet = RipGetNetEntry(networkOrder);
	UNLOCK(RIT.table[RIP_NET_HASH(&networkOrder)].lock, pl);
	if (lNet == NULL) {
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RIPcheckSapSource: localnet for lan is NULL "));
		sapSource->result = -1;
		NTR_VLEAVE();
		return;
	}

	IPXCOPYNET(sapSource->serverAddress, &networkOrder);
	pl = LOCK(RIT.table[RIP_NET_HASH(&networkOrder)].lock, plstr);
	srvrNet = RipGetNetEntry(networkOrder);

	if (srvrNet == NULL) {
		/* I know of no such place... */
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RIPcheckSapSource: no net entry for %X",
				GETINT32(networkOrder)));
		UNLOCK(RIT.table[RIP_NET_HASH(&networkOrder)].lock, pl);
		sapSource->result = -1;
		NTR_VLEAVE();
		return;
	}

	/* am I directly connected to the net being discussed */
		/* Info not coming from direct net */
		/* Unreasonable hop count */
		/* Direct net not star */
		/* But source is not server */
	/* or This message is from the server itself */
	if (srvrNet->netStatus & (uint8)NET_LOCAL_BIT) {
		if ((lNet != srvrNet)
				|| ((sapSource->hops > 1)
					&& (sapSource->hops < MaxHops))
				|| (((srvrNet->netStatus & NET_STAR_BIT) == 0)
					&& (!IPXCMPNODE(&sapSource->serverAddress[4],
								sapSource->reporterAddress)))) {
			NWSLOG((RIPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"RIPcheckSapSource: first failure "));
			UNLOCK(RIT.table[RIP_NET_HASH(&networkOrder)].lock, pl);
			sapSource->result = -1;
			NTR_VLEAVE();
			return;
		}

		UNLOCK(RIT.table[RIP_NET_HASH(&networkOrder)].lock, pl);
		sapSource->result = 0;
		NTR_VLEAVE();
		return;
	}

	/* I am NOT directly connected to this net.  A known router must
	** tell me the service information I seek.
	*/
	srvrRoute = srvrNet->routeListLink;
	while ((srvrRoute != NULL)
			&& (srvrRoute->routeTime == srvrNet->timeToNet)) {
		RIPmapRRKeyToIpxKey(srvrRoute->rrLanKey, (void *)&routeLan);
		if ((sapSource->connectedLan == routeLan) &&
				(IPXCMPNODE(&sapSource->reporterAddress[0],
							&srvrRoute->forwardingRouter[0]))) {
			UNLOCK(RIT.table[RIP_NET_HASH(&networkOrder)].lock, pl);
			/* The "reporter" is a best-source for the target net */
			sapSource->result = 0;
			NTR_VLEAVE();
			return;
		}
		srvrRoute = srvrRoute->nextRouteLink;
	}

	NWSLOG((RIPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
		"RIPcheckSapSource: fall through failure "));
	UNLOCK(RIT.table[RIP_NET_HASH(&networkOrder)].lock, pl);

	sapSource->result = -1;

	NTR_VLEAVE();
	return;
}

/*
 * int RIPgetNetInfo(netInfo_t *netInfo)
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
int
RIPgetNetInfo(netInfo_t *netInfo)
{	netListEntry_t	*lNet;
	uint32	ipxNetAddr;
	pl_t	pl;

	NTR_ENTER(1, netInfo, 0,0,0,0);

	ipxNetAddr = netInfo->netIDNumber;
	pl = LOCK(RIT.table[RIP_NET_HASH(&ipxNetAddr)].lock, plstr);
	lNet = RipGetNetEntry(ipxNetAddr);
	if (lNet == NULL) {
		UNLOCK(RIT.table[RIP_NET_HASH(&ipxNetAddr)].lock, pl);
		return((int)NTR_LEAVE(0));
	}
	netInfo->netStatus = lNet->netStatus;
	netInfo->timeToNet = lNet->timeToNet;
	netInfo->hopsToNet = lNet->hopsToNet;
	if (lNet->routeListLink)
 		RIPmapRRKeyToIpxKey(lNet->routeListLink->rrLanKey,
							(void *)&(netInfo->lanIndex));
	else
		netInfo->lanIndex = -1;
	UNLOCK(RIT.table[RIP_NET_HASH(&ipxNetAddr)].lock, pl);
	return((int)NTR_LEAVE( 1 ));
}

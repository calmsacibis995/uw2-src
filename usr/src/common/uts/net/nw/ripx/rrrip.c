/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ripx/rrrip.c	1.9"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: rrrip.c,v 1.14 1994/08/11 16:33:47 meb Exp $"

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
 * rrrip.c
 *
 * The data and methods to support RIP's part of the Replaceable
 * Router Interface are encapsulated in this module.
 *
 * Lock usage - Code/structures protected:
 *  RRKeysLock - keeps key counts, (de)registrations sane
 * |S|	Add reference or key counter and a lock to protect it
 * |S| or is a timed free sufficient ?
 */
#ifdef _KERNEL_HEADERS
#include <net/nw/ripx/rip.h>		/* ALL, ... */
#include <net/nw/rrouter.h>
#include <net/nw/ripx/rrrip.h>
#else
#include "rip.h"
#include "rrouter.h"
#include "rrrip.h"
#endif /* _KERNEL_HEADERS */

FSTATIC void	*RIPgrantLanKey(rrLanData_t *);
FSTATIC void	 RIPupdateLanKey(void *, rrLanData_t *);
FSTATIC void	*RIPinvalidateLanKey(void *);

FSTATIC void	*RIPsendLanUpdates(void *, void *);

#ifdef OLD_SAP_CHECKING
FSTATIC int		 RIPcheckSapPacket(void *, mblk_t *);
#endif

static void		*rrouterToken = NULL;
extern uint8	AllHosts[];
extern uint16	RipSocket;
extern uint8	routerType;

#define CHANGED_ONLY	0
#define RIP_PACKET_TYPE	1
#define TRANSPORT_CONTROL	0
#define MIN_SERVER_AD_SIZE 96
#define ACCEPT_SERVER_OPERATION	2

/* Lipmx registers a key with a replaceable router (us).
 * We keep some of the lan information (passed as part of
 * that transaction) to help us maintain the Routing Information
 * Table (RIT).  This helps both parties keep their little fingers
 * out of each others work and maintain the abstraction that can
 * help MP-izing go a lot smoother.
 */
typedef struct	rrLanKey {
	void	*ipxLanKey;	/* the key we use to do business with lipmx */
	uint32	net;		/* the IPX network address of the key's lan */
	ipxNode_t	node;	/* the IPX node address of the key's lan */
	uint16	lanTicks;	/* the lan's speed in PC ticks */
	mblk_t	*prepend;	/* pre-header for outgoing packets */
	uint32	addrOffset;	/* where in the pre-header to write data */
	RIPperLanInfo_t	ripCfg;	/* configurable lan data */
	long	lastUpdate;	/* when the last RIP update went to this lan */
	long	ticksPerUpdate;	/* how many clocks between lan updates */
	uint8	lanType;	/* type of lan (LOCAL, STAR, WAN, ...) */
	uint8	state;	/* |S| protect against concurrent Invalidates */
} rrLanKey_t;

extern	lock_t	*ripxDeviceLock;
		mblk_t	*lanKeys = NULL;

FSTATIC void	RIPfreeLanKey(rrLanKey_t *);

typedef struct {
	int		changes;
	long	now;
} updateInfo_t;

/*
 * void RipRegisterRouter(void)
 *
 * After the driver and RIP are sufficiently initialized that
 * it is safe to transact business, this function gets called.
 * This function sets up our part of the RRI and registers with
 * lipmx.  Lipmx, in turn, will register its lans with us in
 * return for keys with which it can then do business with us.
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipRegisterRouter(void)
{	
	RROUTERMethods_t	RIPRouter = {
			RIPgrantLanKey,
			RIPupdateLanKey,
			RIPinvalidateLanKey,
			RIPdigestRouterPacket,
#ifdef OLD_SAP_CHECKING
			RIPcheckSapPacket,
#endif
			RIPmapNetToIpxLanKey,
			RIPgetRouteInfo
	};

	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((RIPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter RipRegisterRouter"));
	rrouterToken
		= RRIPX.RegisterRRouter(&RIPRouter);
	NTR_VLEAVE();
	return;
}

/*
 * void RipDeregisterRouter()
 *
 * from streams.c:
 * As a result of close on control device
 * from ioctls.c:
 * As a result of an M_IOCTL/RIPX_DOWN_ROUTER ioctl
 * Someone with appropriate authority and wisdom has determined
 * that we should stop doing what we do.  Ergo, we will now
 * deregister ourself from lipmx, who in turn will deregister
 * its lan keys with us.
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipDeregisterRouter()
{
	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((RIPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter RipDeregisterRouter"));

	if (rrouterToken) {
		RRIPX.DeregisterRRouter(rrouterToken);
		rrouterToken = NULL;
	}
	NTR_VLEAVE();
	return;
}

/*
 * void * RIPgrantLanKey(rrLanData_t *lanData)
 *
 * Ipx has a lan it wants to do routing with.  We have been
 * passed some information pertinent to that lan and should
 * return to lipmx a key/token it can use to do further business
 * with us.  The key is an opaque pointer: we know about its contents
 * but lipmx doesn't (and shouldn't).
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void *
RIPgrantLanKey(rrLanData_t *lanData)
{	rrLanKey_t	*rrLanKey;
	mblk_t		*keyb;
	pl_t		lvl;

	NTR_ENTER(1, lanData, 0,0,0,0);

	/*
	 *	Use allocb for lan keys, because we cannot be guaranteed
	 *	process context when we free the structure.  Free may
	 *	be the result of a hangup or other stream error.
	 */
	if( (keyb = allocb(sizeof(rrLanKey_t),BPRI_HI)) == NULL ) {
		if(lanData->prepend)
			freemsg(lanData->prepend);
		return((void *)NTR_LEAVE(NULL));
	}
	
	rrLanKey = (rrLanKey_t *)keyb->b_rptr;
	bzero((char *)rrLanKey, sizeof(rrLanKey_t));
	rrLanKey->ipxLanKey = lanData->ipxLanKey;
	rrLanKey->net = lanData->ipxNetAddr;
	rrLanKey->lanTicks = lanData->lanTicks;
	IPXCOPYNODE(&lanData->ipxLanNode, &rrLanKey->node);
	rrLanKey->ripCfg = lanData->ripCfg;
	rrLanKey->ticksPerUpdate
		= rrLanKey->ripCfg.bcastInterval * 30 * HZ;
	rrLanKey->lastUpdate = 0;
	rrLanKey->state = 0;
	/* convert size to max # of RIP entries/packet */
	rrLanKey->ripCfg.maxPktSize
		= (((rrLanKey->ripCfg.maxPktSize - ROUTE_PACKET_SIZE)
						/ ROUTE_ENTRY_SIZE) * ROUTE_ENTRY_SIZE)
			+ ROUTE_PACKET_SIZE;

	/* rrLanKeys only apply to locally connected nets */
	lanData->lanType |= NET_LOCAL_BIT;
	rrLanKey->lanType = lanData->lanType;
	/* hops for an attached network is always 0 */
	if (! RIPAddRouterInfo(lanData->ipxNetAddr,
						(uint8 *)&(lanData->ipxLanNode), 0,
						lanData->lanTicks, rrLanKey,
						lanData->lanType)) {
		if(lanData->prepend)
			freemsg(lanData->prepend);
		NWSLOG((RIPXID,0,PNW_DATA_ASCII,SL_TRACE,
			"RIPgrantLanKey: KMEM: Free lan key structure at 0x%X, size %d", 
				rrLanKey, sizeof(rrLanKey_t)));
		freeb(keyb);
		return((void *)NTR_LEAVE(NULL));
	}
	/* see if we need to change the # of 30-sec
	** intervals at which we fire off aging.
	*/
	RipAdjustTimerInterval(rrLanKey->ripCfg.bcastInterval);
	rrLanKey->prepend = lanData->prepend;
	rrLanKey->addrOffset = lanData->addrOffset;

	/*
	 *	Save pointers to mblks so we can free them later
	 */
	lvl = LOCK( ripxDeviceLock, plstr);
	keyb->b_cont = rrLanKey->prepend;
	keyb->b_next = lanKeys;
	lanKeys = keyb;
	UNLOCK( ripxDeviceLock, lvl);

	/* rrLanKey is valid
	*/
	return((void *)NTR_LEAVE(rrLanKey));
}

/*
 * void RIPupdateLanKey(void *key, rrLanData_t *lanData)
 *
 * Update the data associated with this rrLanKey.
 * Currently only affects whether or not RIP info is consumed/sent.
 * Any other changes currently need to be done by
 * Invalidate/Grant combo.
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RIPupdateLanKey(void *key, rrLanData_t *lanData)
{	rrLanKey_t	*rrLanKey = (rrLanKey_t *)key;

/* ASSERT */
	NTR_ENTER(1, lanData, 0,0,0,0);

	if ((rrLanKey != NULL) && (lanData != NULL)) {
		rrLanKey->ripCfg.actions
			= lanData->ripCfg.actions;
	} else {
		NWSLOG((RIPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
				"RIPupdateLanKey: called with NULL key or data"));
	}

	NTR_VLEAVE();
	return;
}

/*
 * void * RIPinvalidateLanKey(void *key)
 *
 * When a lan is going away, this gets called to remove its
 * net entry from the RIP table and announce its demise
 * Called from lipmx.c across RRI
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void *
RIPinvalidateLanKey(void *key)
{	rrLanKey_t	*rrLanKey = (rrLanKey_t *)key;

	NTR_ENTER(1, rrLanKey, 0,0,0,0);

/* |S| ASSERT, protect against concurrent instances
 */
	if ((rrLanKey != NULL) && (rrLanKey->state == 0)) {
		NWSLOG((RIPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
				"RIPinvalidateLanKey: kill ipxLanKey 0x%X net 0x%X",
				rrLanKey->ipxLanKey, GETINT32(rrLanKey->net)));
		rrLanKey->state++;
		RipDeconfigureLocalLan(rrLanKey, rrLanKey->net);

		/*
		 * All lan keys are linked together, free them all at one at close
		 * time.
		 */
	} else {
		NWSLOG((RIPXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
				"RIPinvalidateLanKey: called with NULL key"));
	}
	return((void *)NTR_LEAVE(NULL));
}

/*
 * netListEntry_t * RIPLanData(void *key, void **ipxLanKeyPtr,
 *								uint8 *node, uint16 *maxAgePtr)
 *
 * from rip.c:
 * RIPAddRouterInfo
 * RipSendRouterInfo
 * RipRequestRouteInfo
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
uint32
RIPLanData(void *key, void **ipxLanKeyPtr, uint8 *node, uint16 *maxAgePtr)
{	rrLanKey_t	*rrLanKey = (rrLanKey_t *)key;

	NTR_ENTER(4, key, ipxLanKeyPtr, node, maxAgePtr, 0);

	if (rrLanKey) {
		if (node)
			IPXCOPYNODE(&rrLanKey->node, node);
		*ipxLanKeyPtr = rrLanKey->ipxLanKey;
		if (maxAgePtr) {
			*maxAgePtr = rrLanKey->ripCfg.ageOutIntervals
							* rrLanKey->ripCfg.bcastInterval;
			NWSLOG((RIPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
					"maxAge gets %d bcastIntervals for ipxLanKey 0x%X",
					*maxAgePtr, *ipxLanKeyPtr));
		}
		return((uint32)NTR_LEAVE(rrLanKey->net));
	} else
		return((uint32)NTR_LEAVE(0));
}

/*
 * uint8 RIPNetStatus(void *rrLanKey)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
uint8
RIPNetStatus(void *rrLanKey)
{
	NTR_ENTER(1, rrLanKey, 0,0,0,0);
	return((uint8)NTR_LEAVE(((rrLanKey_t *)rrLanKey)->lanType));
}

/*
 * uint16 RipMaxLanPkt(void *key)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
uint16
RipMaxLanPkt(void *key)
{	rrLanKey_t	*rrLanKey = (rrLanKey_t *)key;

	NTR_ENTER(1, key, 0,0,0,0);
	return((uint16)NTR_LEAVE(rrLanKey ? rrLanKey->ripCfg.maxPktSize : 0));
}

/*
 * uint16 RipLanTime(void *key)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
uint16
RipLanTime(void *key)
{	rrLanKey_t	*rrLanKey = (rrLanKey_t *)key;

	NTR_ENTER(1, key, 0,0,0,0);
	return((uint16)NTR_LEAVE(rrLanKey->lanTicks));
}

/*
 * void RIPSendUpdates(int changes, long now)
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RIPSendUpdates(int changes, long now)
{	updateInfo_t	info;

	NTR_ENTER(2, changes, now, 0,0,0);

	info.now = now;
	info.changes = changes;
	(void)RRIPX.UseMethodWithLanKey(ALL_LANS, (void *)&info,
										RIPsendLanUpdates);
	RIPClearChanged();
	NTR_VLEAVE();
	return;
}

/*
 * void RipLanDeathScream(void *routerKey)
 *
 * The lan being born/killed needs to let the lan know that
 * the rest of the world can (now)/(no longer) be seen from this router.
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipLanDeathScream(void *rrKey)
{	rrLanKey_t		*rrLanKey = (rrLanKey_t *)rrKey;

	NTR_ENTER(1, rrLanKey, 0,0,0,0);

	if ((routerType & CLIENT_ROUTER) ||
			(!(rrLanKey->ripCfg.actions & SEND_RIPS))) {
		NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
				"RipLanDeathScream: ipxLanKey 0x%X doesn't send RIPs",
				rrLanKey->ipxLanKey));
		NTR_VLEAVE();
		return;
	}
	RipSendRouterInfo(rrLanKey,(uint8 *)&rrLanKey->net,
									AllHosts, NULL, FULL_INFO);
	NTR_VLEAVE();
	return;
}

/*
 * void * RIPsendLanUpdates(void *routerKey, void *lKey)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void *
RIPsendLanUpdates(void *routerKey, void *lKey)
{	rrLanKey_t		*rrLanKey = (rrLanKey_t *)lKey;
	updateInfo_t	*info = (updateInfo_t *)routerKey;
	int action;

	NTR_ENTER(2, routerKey, lKey, 0,0,0);

	if ((routerType & CLIENT_ROUTER) ||
			(!(rrLanKey->ripCfg.actions & SEND_RIPS))) {
		NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
				"RIPsendLanUpdates: ipxLanKey 0x%X doesn't send RIPs",
				rrLanKey->ipxLanKey));
		return((void *)NTR_LEAVE(NULL));
	}
	if ((info->now - rrLanKey->lastUpdate) >= rrLanKey->ticksPerUpdate) {
		if (info->changes) {
			if( rrLanKey->ripCfg.actions & RIP_UPDATES_ONLY) {
				action = CHANGED_ONLY;
			} else {
				action = FULL_INFO;
			}
			RipSendRouterInfo(rrLanKey,(uint8 *)&rrLanKey->net,
								AllHosts, NULL, action);
			rrLanKey->lastUpdate = info->now;
		} else {
			if ( !(rrLanKey->ripCfg.actions & RIP_UPDATES_ONLY)) {
				RipSendRouterInfo(rrLanKey,(uint8 *)&rrLanKey->net,
									AllHosts, NULL, FULL_INFO);
				rrLanKey->lastUpdate = info->now;
			}
		}
	} else {
		if (info->changes) {
			RipSendRouterInfo(rrLanKey,(uint8 *)&rrLanKey->net,
								AllHosts, NULL, CHANGED_ONLY);
		}
	}
	return((void *)NTR_LEAVE(NULL));
}

/*
 * void * RIPSendRouterInfoSWITCH(void *routerKey, void *lKey)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void *
RIPSendRouterInfoSWITCH(void *routerKey, void *lKey)
{	uint32		flag = (uint32) routerKey;
	rrLanKey_t	*rrLanKey = (rrLanKey_t *)lKey;

	NTR_ENTER(2, routerKey, lKey, 0,0,0);

	if ((routerType & CLIENT_ROUTER) ||
			(!(rrLanKey->ripCfg.actions & SEND_RIPS))) {
		NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
				"RIPSendRouterInfoSWITCH: ipxLanKey 0x%X doesn't send RIPs",
				rrLanKey->ipxLanKey));
		return((void *)NTR_LEAVE(NULL));
	}
/* ASSERT */
	switch(flag) {
		case ADVERTISE:
			NWSLOG((RIPXID,0,PNW_ALL,SL_TRACE,
				"RIPSendRouterInfoSwitch: Advertising on ipxLanKey %x.",
				rrLanKey->ipxLanKey));
			/*
			** If must advertise send ALL else send CHANGED_ONLY
			*/
			RipSendRouterInfo(rrLanKey, (uint8 *)&rrLanKey->net,
								AllHosts, NULL, NET_REFER);
			break;
		case POST_LOST:
			if ((rrLanKey->lanType & NET_RELIABLE_BIT) == 0)
				RipSendRouterInfo(rrLanKey, (uint8 *)&rrLanKey->net,
									AllHosts, NULL, CHANGED_ONLY);
			break;
		case ACCEPT_INFO:
			RipSendRouterInfo(rrLanKey, (uint8 *)&rrLanKey->net,
								AllHosts, NULL, CHANGED_ONLY);
			break;
		case AGE_SLOW:
			if ((rrLanKey->lanType & NET_RELIABLE_BIT))
				RipSendRouterInfo(rrLanKey, (uint8 *)&rrLanKey->net,
									AllHosts, NULL, CHANGED_ONLY);
			break;
		case AGE_NORMAL:
			if ((rrLanKey->lanType & NET_RELIABLE_BIT) == 0)
				RipSendRouterInfo(rrLanKey, (uint8 *)&rrLanKey->net,
									AllHosts, NULL, FULL_INFO);
			break;
		case DOWN_ROUTER:
			RipSendRouterInfo(rrLanKey, (uint8 *)&rrLanKey->net,
								AllHosts, NULL, FULL_INFO);
			break;
		default:
			break;
	}
	return((void *)NTR_LEAVE(NULL));
}

/*
 * mblk_t * RIPmakeDlHdr(ipxHdr_t *hdr, void **ipxLanKeyPtr,
 *							netRouteEntry_t *route)
 *
 * from ripx.c
 *
 * Calling/Exit State:
 *  Lock held around call on route's net entry's bucket
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
mblk_t *
RIPmakeDlHdr(ipxHdr_t *hdr, void **ipxLanKeyPtr, netRouteEntry_t *route)
{	mblk_t  *preHdr;
	uint8	*d;

	NTR_ENTER(3, hdr, ipxLanKeyPtr, route, 0,0);

	if ((preHdr = ((rrLanKey_t *)(route->rrLanKey))->prepend) != NULL) {
		if ((preHdr = copymsg(preHdr)) == NULL) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
					"RIPmakeDlHdr: copymsg failed size = 0x%X",
					DATA_SIZE(((rrLanKey_t *)(route->rrLanKey))->prepend)));
			*ipxLanKeyPtr = (void *)-1;
		} else {
			/* If dest net is local, this is the last router the pkt will
			** cross.  Copy node address of ultimate target into dlHdr.
			** If pkt is destined for a remote net, target pkt for next
			** router.
			*/
			d = preHdr->b_rptr
					+ ((rrLanKey_t *)(route->rrLanKey))->addrOffset;
			if (IPXCMPNET(hdr->dest.net,
							&(((rrLanKey_t *)(route->rrLanKey))->net))) {
				IPXCOPYNODE(hdr->dest.node, d);
			} else {
				IPXCOPYNODE(route->forwardingRouter, d);
			}
			*ipxLanKeyPtr = ((rrLanKey_t *)(route->rrLanKey))->ipxLanKey;
		}
	} else {
		NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
				"RIPmakeDlHdr: ipxLanKey 0x%X net 0x%X has no dlHdr to copy",
				((rrLanKey_t *)(route->rrLanKey))->ipxLanKey,
				GETINT32(((rrLanKey_t *)(route->rrLanKey))->net)));
		*ipxLanKeyPtr = ((rrLanKey_t *)(route->rrLanKey))->ipxLanKey;
	}
	return((mblk_t *)NTR_LEAVE(preHdr));
}

/*
 * void RipDispatchPkt(void *rrLanKey, mblk_t *mp, uint8 flags)
 *
 * from rip.c:
 * RipGiveRouterInfo
 * RipSendRouterInfo
 * RipRequestRouteInfo
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
void
RipDispatchPkt(void *rrLanKey, mblk_t *mp, uint8 flags)
{	ipxHdr_t	*ipxHdr = (ipxHdr_t *)mp->b_rptr;
	mblk_t  *preHdr;

	NTR_ENTER(3, rrLanKey, mp, flags, 0,0);

	/* ASSERT - rrLanKey is never NULL */

	MTYPE(mp) = M_DATA;
	PPUTINT16(IPX_CHKSUM, &ipxHdr->chksum);
	ipxHdr->tc = TRANSPORT_CONTROL;
	ipxHdr->pt = RIP_PACKET_TYPE;

	IPXCOPYNET(&(((rrLanKey_t *)rrLanKey)->net), ipxHdr->dest.net);
	IPXCOPYNET(&(((rrLanKey_t *)rrLanKey)->net), ipxHdr->src.net);
	IPXCOPYNODE(&(((rrLanKey_t *)rrLanKey)->node), ipxHdr->src.node);
	IPXCOPYSOCK(&RipSocket, ipxHdr->src.sock);

	if ((preHdr = ((rrLanKey_t *)rrLanKey)->prepend) != NULL) {
		if ((preHdr = copymsg(preHdr)) == NULL) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
					"RipDispatchPkt: copymsg failed size = 0x%X",
					DATA_SIZE(((rrLanKey_t *)rrLanKey)->prepend)));
			freemsg(mp);
			NTR_VLEAVE();
			return;
		}
		IPXCOPYNODE(ipxHdr->dest.node,
					preHdr->b_rptr
						+ ((rrLanKey_t *)rrLanKey)->addrOffset);
		preHdr->b_cont = mp;
		mp = preHdr;
	} else {
		NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
				"RipDispatchPkt: ipxLanKey 0x%X net 0x%X has no dlHdr to copy",
				((rrLanKey_t *)rrLanKey)->ipxLanKey,
				GETINT32(((rrLanKey_t *)rrLanKey)->net)));
	}
	RRIPX.SendData(mp, &(((rrLanKey_t *)rrLanKey)->ipxLanKey), flags);

	NTR_VLEAVE();
	return;
}

/*
 * int RIPmapRRKeyToIpxKey(void *rrLanKey, void **ipxLanKeyPtr)
 *
 * from rip.c:
 * RIPcheckSapSource is the heaviest trafficker here
 * RIPIGetRouterTable
 * RIPgetNetList
 * RIPmapNetToIpxLanKey
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
int
RIPmapRRKeyToIpxKey(void *rrLanKey, void **ipxLanKeyPtr)
{
	NTR_ENTER(2, rrLanKey, ipxLanKeyPtr, 0,0,0);

	*ipxLanKeyPtr = ((rrLanKey_t *)rrLanKey)->ipxLanKey;

	NWSLOG((RIPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"RIPmapRRKeyToIpxKey: rrLanKey 0x%X yields ipxLanKey 0x%X",
			rrLanKey, *ipxLanKeyPtr));
	return((int)NTR_LEAVE( 1 ));
}

/*
 * int RIPChkNetConflict(void *key, uint8 *sNet, uint8 *dNet)
 *
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
int
RIPChkNetConflict(void *key, uint8 *sNet, uint8 *dNet)
{	rrLanKey_t			*rrLanKey;
	uint32				ipxAddr1;
	uint16				ipxAddr2;
	uint32 				srcNet, dstNet;

	NTR_ENTER(3, key, sNet, dNet, 0,0);

	if (!(IPXCMPNET(sNet,dNet))) {
		IPXCOPYNET(sNet, &srcNet);
		IPXCOPYNET(dNet, &dstNet);
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RIPChkNetConflict: conflict: net src 0x%X != dest 0x%X",
			srcNet, dstNet));
		return(NTR_LEAVE(0));
	}


	rrLanKey = (rrLanKey_t *)key;
/* ASSERT */
if (rrLanKey == NULL) {
	return(NTR_LEAVE(0));
}
	if (!rrLanKey->net) {
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
			"RIPChkNetConflict: Local Net NULL for ipxLanKey 0x%X",
			rrLanKey->ipxLanKey));
		return(NTR_LEAVE(0));
	}

	if (!(rrLanKey->lanType & (NET_STAR_BIT | NET_WAN_BIT))
			&& (!IPXCMPNET(&rrLanKey->net, sNet))) {
		/*
		** There is another router that DISAGREES with me
		** about the net number of one of my nets !!!!
		*/
		GETALIGN32(&rrLanKey->node,&ipxAddr1);
		GETALIGN16(&(((uint8 *)&(rrLanKey->node))[4]), &ipxAddr2);
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"RIPChkNetConflict: Router Configuration Error! Router %x%x",
				ipxAddr1, ipxAddr2));
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
				"RIPChkNetConflict: claims LAN 0x%X is net 0x%X",
				rrLanKey->ipxLanKey, PGETINT32(sNet)));
		return(NTR_LEAVE(0));
	}

	/*
	** Ethernets have been illegally echoing send packets back into
	** the router code, causing serious havoc.  Test for loopback and
	** scream if it's detected.
	*/
	/* Filtering of NIC broadcast echoes is now being done in lipmx
	*/
	return(NTR_LEAVE( 1 ));	/* looks okay */
}

#ifdef OLD_SAP_CHECKING
/*
 * int RIPcheckSapPacket(void *rrLanKey, mblk_t *mp)
 *
 * route and sap packet are similiar in design
 * - thats why we use ROUTE constants & typedefs
 *
 * Calling/Exit State:
 *
 *
 * Lock usage - Code/structures protected:
 *
 */
int
RIPcheckSapPacket(void *rrLanKey, mblk_t *mp)
{	routePacket_t	*routeHdr;
	uint16			len;
	uint16			operation;

	NTR_ENTER(2, rrLanKey, mp, 0,0,0);

	if (!rrLanKey) {
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
				"RIPcheckSapPacket: NULL rrLanKey !!"));
		return(NTR_LEAVE(0));
	}

	routeHdr = (routePacket_t *)mp->b_rptr;
	len = PGETINT16(&routeHdr->len);
	if (len < ROUTE_PACKET_SIZE)
		return(NTR_LEAVE(0));

	GETALIGN16(&routeHdr->operation, &operation);
	if ((GETINT16(operation) == ACCEPT_SERVER_OPERATION)
			&& (len >= MIN_SERVER_AD_SIZE))
		/*
		** If there is a network conflict, drop this sap packet.
		** Otherwise pass it upstream.
		*/
		return(NTR_LEAVE(RIPChkNetConflict(rrLanKey, 
						routeHdr->src.net, routeHdr->dest.net)));
	return(NTR_LEAVE( 1 ));
}
#endif  /* OLD_SAP_CHECKING */

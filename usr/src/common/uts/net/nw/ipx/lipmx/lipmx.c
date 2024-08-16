/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/lipmx/lipmx.c	1.19"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: lipmx.c,v 1.26.2.4.2.2 1995/01/31 20:41:37 vtag Exp $"

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

#ifdef _KERNEL_HEADERS
#include <net/nw/ipx/lipmx/lipmx.h>
#include <net/nw/ipx/lipmx/lipmx_lans.h>
#include <net/nw/ipx/lipmx/rripx.h>
#include <net/nw/ipx/lipmx/lipmx_ioctls.h>
#include <net/nw/ipx/lipmx/norouter.h>
#else
#include "lipmx.h"
#include "lipmx_lans.h"
#include "rripx.h"
#include "lipmx_ioctls.h"
#include "norouter.h"
#endif /* _KERNEL_HEADERS */

static Lan_t	*LipmxLans = NULL;
static uint32	ConfiguredLans = 0;
static uint16	IpxMaxHops;

extern	int		strmsgsz;

lock_t			*sapQLock =  NULL;
queue_t			*sapQ = NULL;	/* q for sapping when !underIpx */
atomic_int_t	sapQcount;		/* count of putnext calls active for sapQ */

LKINFO_DECL(lipmxLanLkinfo, "limpxLanLock", 0);

static const uint8	ALLHOSTS[6] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF};
static const uint8	ZEROHOST[6] = {0,0,0,0,0,0};
static const uint16	SapSocket = GETINT16(SAP_SOCKET);
static const uint16	RipSocket = GETINT16(RIP_SOCKET);
static const uint16	diagSocket = GETINT16(IPX_DIAGNOSTIC_SOCKET);

FSTATIC int		 LipmxGetDlMp(uint32, mblk_t **);
FSTATIC int		 LipmxBuildRRlanKey(uint32, pl_t);
FSTATIC void	 LipmxRoutePacket(queue_t *, mblk_t *);
FSTATIC void	 LipmxIpxDiagnostics(mblk_t *, uint32);
FSTATIC void	 LipmxInitLanData(uint32 lan);
FSTATIC void	 LipmxInitLocalNets(void);
FSTATIC mblk_t	*LIPMXTakeItOff( mblk_t **);

/*
 *	Flag set if lan0 accepting rip packets
 */
static int	RouteRipToIpx = DISABLE_RIP;

/*
 * void lipmxinit(void)
 *	Called during driver load. Set up default router (NoRouter).
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
lipmxinit(void)
{

	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,"Enter lipmxinit"));

	/*
	 *+ Notification that driver is loaded
	 */
#ifdef DEBUG
	cmn_err(CE_CONT, "%s %s%s: %s %s\n",
						LIPMXSTR, LIPMXVER, LIPMXVERM, __DATE__, __TIME__);
#else
	cmn_err(CE_CONT, "%s %s%s\n", LIPMXSTR, LIPMXVER, LIPMXVERM);
#endif /* DEBUG */

	lipmxStats.LipmxMajorVersion = (uint8)LIPMX_MAJOR;
	lipmxStats.LipmxMinorVersion = (uint8)LIPMX_MINOR;
	lipmxStats.LipmxRevision[0] = (char)LIPMX_REV1;
	lipmxStats.LipmxRevision[1] = (char)LIPMX_REV2;

	/* Set up the default null router */
	LipmxInitNoRouter();

	if( lipmxStats.StartTime == 0) {
		drv_getparm( TIME, &lipmxStats.StartTime);
	}

	NTR_VLEAVE();
	return;
}

/*
 * void lipmxCleanUp(void)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
void
lipmxCleanUp(void)
{

	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,"Enter lipmxCleanUp"));

	LipmxInitLocalNets();

	NTR_VLEAVE();
	return;
}

/*
 * void LipmxStreamError(queue_t *q, mblk_t *mp)
 *	M_ERROR or M_HANGUP came up from a lan driver
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LipmxStreamError(queue_t *q, mblk_t *mp)
{	
	Lan_t	*lanInfo = (Lan_t *)q->q_ptr;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxStreamError: Bad Stream on Lower Read Ipx"));
	if (lanInfo == NULL) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxStreamError: q->q_ptr is NULL"));
		NTR_VLEAVE();
		return;
	}
#ifdef DEBUG
	/*
	 *+ We got a Stream Error M_ERROR, or M_HANGUP
	 */
	cmn_err(CE_WARN, "LipmxStreamError: M_ERROR or M_HANGUP received");
#endif /* DEBUG */
	/* just invalidate lanKey ? */
	freemsg(mp);
	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	lanInfo->rrLanKey = RROUTER.InvalidateLanKey(lanInfo->rrLanKey);
	ATOMIC_INT_DECR(&RRIPX.Ripcount);
	NTR_VLEAVE();
	return;
}

/*
 *	mblk_t *LipmxTrimPacket( mblk_t *mp)
 *		Trims padding from packets
 *
 *	Calling/Exit State:
 *		No locks required on entry
 *		No locks required on exit
 */
mblk_t *
LipmxTrimPacket( mblk_t *mp)
{
	uint16	dataLen;
	ipxHdr_t    *ipxHeader = (ipxHdr_t *)mp->b_rptr;
	mblk_t *mpt = mp;
	int alterflag = 0;

	NTR_ENTER(1, mp, 0,0,0,0);
	/*
	 * Does this message have padding that needs to be trimmed ?
	 * first check if the whole message is in one block
	 * if so, reset the write pointer and free any continuation
	 * blocks if they exist.
	 */
	dataLen = PGETINT16(&(ipxHeader->len));
	if( (uint16)DATA_SIZE( mp) >= dataLen) {
		mp->b_wptr = mp->b_rptr + dataLen;
		if( mp->b_cont != NULL) {
			lipmxStats.TrimPacket++;
			freemsg( mp->b_cont);
			mp->b_cont = NULL;
		}
	} else {
		/*
		 * This message spans multiple blocks.  We will adjust the
		 * write pointer in the appropriate block and free any
		 * additional blocks.
		 */
		while( mpt->b_cont != NULL) {
			dataLen -= DATA_SIZE(mpt);
			mpt = mpt->b_cont;
			if( (uint16)DATA_SIZE(mpt) >= dataLen) {
				alterflag++;
				mpt->b_wptr = mpt->b_rptr + dataLen;
				if( mpt->b_cont != NULL) {
					freemsg( mpt->b_cont);
					mpt->b_cont = NULL;
				}
			}
		}

		/*
		 * Check to see if ipx packet length is greater than the amount
		 * of actual data. if so bad length in ipx packet
		 */
		if( dataLen > (uint16)DATA_SIZE(mpt) ) {
			NWSLOG((LIPMXID, 0, PNW_DATA_ASCII, SL_TRACE,
				"LipmxTrimPacket: bad ipx packet length %d, msg length %d, dropped",
				(uint16)(PGETINT16(&(ipxHeader->len))),  msgdsize(mp)) );
			lipmxStats.InBadLength++;
			freemsg(mp);
			return((int)NTR_LEAVE(NULL));
		}
		if( alterflag == 0) {
			/*
			 * We ran out of data before the ipx data length was
			 * satisfied.  This is a bad packet, drop it
			 */
			lipmxStats.InBadLength++;
			NWSLOG((LIPMXID, 0, PNW_DATA_ASCII, SL_TRACE,
				"LipmxTrimPacket: cannot trim message, length %d, need %d, dropped",
				msgdsize(mp), (uint16)(PGETINT16(&(ipxHeader->len)))) );
			freemsg(mp);
			mp = NULL;
		} else {
			lipmxStats.TrimPacket++;
		}
	}
	NTR_LEAVE( (uint32)mp);
	return( mp);
	
}

/*
 * void LipmxDlUnitdataInd(queue_t *q, mblk_t *mp)
 *	Called from lipmxlrput(), this function handles the only
 *	interesting case (ie; all data).
 *
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LipmxDlUnitdataInd(queue_t *q, mblk_t *mp)
{	mblk_t 		*tmpMp;
	Lan_t		*lanInfo = (Lan_t *)q->q_ptr;
	uint16		inboundSocket;
	uint32		srcNetAddr;
	uint32		destNetAddr;
	uint32		lan;
	ipxHdr_t	*ipxHeader;
	pl_t		lvl;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LipmxDlUnitdataInd"));
	if (lanInfo == NULL) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxDlUnitdataInd: Drop message, q->q_ptr is NULL"));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}
	/*
	** A message that enters this routine should have a Link Layer
	** Header in the first message block (of type M_PROTO, checked
	** before we got here) and then the Data in the message block
	** pointed to by mp->b_cont.  If no Data message is present,
	** drop the packet and return.
	*/
	lipmxStats.InTotal++;

	if (mp->b_cont == NULL) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxDlUnitdataInd: Drop message with no data"));
		freemsg(mp);
		lipmxStats.InBadLength++;
		NTR_VLEAVE();
		return;
	}

	/*
	** Remove the Link Layer Header.
	*/
	tmpMp = mp;
	mp = mp->b_cont;
	freeb(tmpMp);
	MTYPE(mp) = M_DATA;

	/*
	** Make sure the Data is at least big enough to hold an IPX
	** header before trying to de-reference the data.
	**
	** This guarantees that upstream drivers or modules
	** will have at least IPX_HDR_SIZE in the first mp, it
	** does not however guarantee alignment
	*/
	if (DATA_SIZE(mp) < IPX_HDR_SIZE) {
		if (pullupmsg(mp, IPX_HDR_SIZE) == 0) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxDlUnitdataInd: Drop bad packet : size 0x%x",
				msgdsize(mp)));
			freemsg(mp);
			lipmxStats.InBadLength++;
			NTR_VLEAVE();
			return;
		}
		lipmxStats.InCoalesced++;
	}
	ipxHeader = (ipxHdr_t *)mp->b_rptr;

	NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"LipmxDlUnitdataInd: Inbound Socket = %xh",
		PGETINT16(&ipxHeader->dest.sock)));

	/*
	** If the source network is 0 then fill it in with the
	** network that the packet came in on.
	*/
	IPXCOPYNET(ipxHeader->src.net,&srcNetAddr);
	if (srcNetAddr == 0) {
		IPXCOPYNET(&lanInfo->ipxNet,ipxHeader->src.net);
	}

	/*
	** If the destination network is 0 then fill it in with the
	** network that the packet came in on.
	*/
	IPXCOPYNET(ipxHeader->dest.net,&destNetAddr);
	if (destNetAddr == 0) {
		IPXCOPYNET(&lanInfo->ipxNet,ipxHeader->dest.net);
		IPXCOPYNET(ipxHeader->dest.net,&destNetAddr);
	}

	/*
	** Check the packet type to see if this is a Net Bios
	** interlan broadcast packet.  If so, duplicate the
	** packet and send to each network that it has not been on.
	*/
	if (ipxHeader->pt == IPX_PROPAGATION_TYPE) {
		LipmxDupBroadcastPacket(q, mp);
		NTR_VLEAVE();
		return;
	}

	/*
	** De-reference the IPX header and extract the destination
	** socket. (inboundSocket)
	*/
	IPXCOPYSOCK(ipxHeader->dest.sock,&inboundSocket);

#ifdef DRIVER_ECHO
	if (IPXCMPNET(&lanInfo->ipxNet, ipxHeader->src.net)
			&& IPXCMPNODE(lanInfo->ipxNode, ipxHeader->src.node)) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxDlUnitdataInd: Drop bcast echoed by NIC"));
		freemsg(mp);
		lipmxStats.InDriverEcho++;
		NTR_VLEAVE();
		return;
	}
#endif

	/*
	** Check the socket to see if it is the ROUTER SOCKET.
	** If it is, execute the router code but do not throw
	** away the router packet.  Hold on to it long enough
	** to see if the router socket is open and if so pass the
	** message upstream also (only if someone accepting rip pkts).
	*/
	if (inboundSocket == RipSocket) {
		lipmxStats.InRip++;
		if ((uint16)DATA_SIZE(mp) < (uint16)PGETINT16(&ipxHeader->len)) {
			if (pullupmsg(mp, PGETINT16(&ipxHeader->len)) != 0) {
				/* must guarantee contiguous msg to Replaceable router
				*/
				ATOMIC_INT_INCR(&RRIPX.Ripcount);
				RROUTER.DigestRouterPacket(lanInfo->rrLanKey, mp);
				ATOMIC_INT_DECR(&RRIPX.Ripcount);
				/* Reset ipxHeader after pullupmsg */
				ipxHeader = (ipxHdr_t *)mp->b_rptr;
			} else {
				/* If pullup failed, send up anyway.  User process will
				** have to check pkt len against bytes rcvd to debug.
				*/
		   		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			   		"LipmxDlUnitdataInd: pullup of RIP pkt failed"));
			}
		} else {
			/* must guarantee contiguous msg to Replaceable router
			*/
			ATOMIC_INT_INCR(&RRIPX.Ripcount);
			RROUTER.DigestRouterPacket(lanInfo->rrLanKey, mp);
			ATOMIC_INT_DECR(&RRIPX.Ripcount);
		}

		if (RouteRipToIpx == ENABLE_RIP) {
			if( LipmxTrimPacket( mp) != NULL) {
				++lipmxStats.InRipRouted;
				IpxRouteDataToSocket( mp);
			}
		} else {
			++lipmxStats.InRipDropped;
			NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
				"LipmxDlUnitdataInd: Drop RIP - no socket to go to"));
			freemsg(mp);
		}
		NTR_VLEAVE();
		return;
	}

	if( LipmxTrimPacket( mp) == NULL) {
		NTR_VLEAVE();
		return;
	}

	if (inboundSocket == SapSocket) {
		lipmxStats.InSap++;
		/* if source net, destination net, and the net that this sap packet
		 * came in on is not all the same, toss the sap packet.
		 */
		if( !(IPXCMPNET(&lanInfo->ipxNet,ipxHeader->dest.net)) ||
			!(IPXCMPNET(ipxHeader->src.net,ipxHeader->dest.net))) {

			NWSLOG((LIPMXID,0,PNW_DROP_PACKET,SL_TRACE,
				"DlUnitInd: Bad sap source, snet= 0x%X dnet= 0x%X inNet= 0x%X",
				PGETINT32(&ipxHeader->src.net), PGETINT32(&ipxHeader->dest.net),
				PGETINT32(&lanInfo->ipxNet) ));
			lipmxStats.InSapBad++;
			freemsg(mp);
		} else {
			if (lipmxStats.IpxSocketMux) {
				lipmxStats.InSapIpx++;
				IpxRouteDataToSocket( mp);
			} else {
				if (lanInfo->ipxNet == destNetAddr) {
					ASSERT(sapQLock != NULL);
					lvl = LOCK(sapQLock, plstr);
					if (sapQ) {
						if (canputnext(sapQ)) {
							ATOMIC_INT_INCR(&sapQcount);
							lipmxStats.InSapNoIpxToSapd++;
							UNLOCK(sapQLock, lvl);
							putnext(sapQ, mp);
							ATOMIC_INT_DECR(&sapQcount);
						} else {
							UNLOCK(sapQLock, lvl);
							NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
								"LipmxDlUnitdataInd: canput failed to sapQ"));
						}
					} else {
						UNLOCK(sapQLock, lvl);
						NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
							"LipmxDlUnitdataInd: SAP packet and not under ipx, drop packet"));
						lipmxStats.InSapNoIpxDrop++;
						freemsg(mp);
					}
				}
			}
		}
		NTR_VLEAVE();
		return;
	}

	/*
	**	This is a shortcut for the internal network.  We test destination
	**	net and node.  If they match the internal net/node we just send
	**	this packet to the socket multiplexor, thus avoiding RIP overhead
	**	to determine the destination net
	*/
	if( (LipmxLans[0].ipxNet == destNetAddr)
			&& IPXCMPNODE(LipmxLans[0].ipxNode, ipxHeader->dest.node) ) {
		lipmxStats.InInternalNet++;
		IpxRouteDataToSocket( mp);
		NTR_VLEAVE();
		return;
	}

	/*
	** See if the the incoming packet is for a local network.
	** If not, then route it to the next local target.
	*/
	/* query router, see if local, bypass scan */
	for (lan = 0; lan < ConfiguredLans; lan++) {
		if (LipmxLans[lan].ipxNet == destNetAddr)
			break;
	}
	if (lan >= ConfiguredLans) {
		/*
		** This packet is for a lan which is not
		** locally connected.
		*/
		lipmxStats.InForward++;
		LipmxRoutePacket(q, mp);
		NTR_VLEAVE();
		return;
	}
	/*
	** This packet is to one of my local nets.
	** If it is for my internal network, receive the packet.
	** If not, see if it is a directed broadcast.  If it is,
	** then route it the correct network if doing so will not
	** exceed the max number of routers traversed.
	** Otherwise, throw it away and return.
	*/
	if (IPXCMPNODE(LipmxLans[lan].ipxNode,ipxHeader->dest.node)) {
		if (lan == 0) {	/* internal network */
			if (inboundSocket == diagSocket) {
				lipmxStats.InDiag++;
				lipmxStats.InDiagInternal++;
			} else {
				lipmxStats.InInternalNet++;
			}
			IpxRouteDataToSocket( mp);
		} else {
			if (inboundSocket == diagSocket) {
				/* For Netware Management Services (NMS) to work
				** with IPX Diagnostics, the bridge must re-direct
				** IPX Diagnostics requests sent
				** to router cards up to the internal net.
				*/
				lipmxStats.InDiagNIC++;
				lipmxStats.InDiag++;
				if (lipmxStats.IpxSocketMux) {
					IPXCOPYNET(&LipmxLans[0].ipxNet,
								ipxHeader->dest.net);
					IPXCOPYNODE(LipmxLans[0].ipxNode,
								ipxHeader->dest.node);
					lipmxStats.InDiagIpx++;
					IpxRouteDataToSocket( mp);
				} else {
					lipmxStats.InDiagNoIpx++;
					LipmxIpxDiagnostics(mp,lan);
				}
			} else {
				NWSLOG((LIPMXID,0,PNW_DROP_PACKET,SL_TRACE,
					"LipmxDlUnitdataInd: Drop pkt sent to router card"));
				lipmxStats.InNICDropped++;
				freemsg(mp);
			}
		}
	} else { /* Not directed to any node on this box */

		if (!IPXCMPNODE(ipxHeader->dest.node,ALLHOSTS)) {
			/* Not a broadcast, so no node here is target */
			lipmxStats.InRoute++;
			LipmxRoutePacket(q, mp);
			NTR_VLEAVE();
			return;
		}

		/* This is a broadcast */
		lipmxStats.InBroadcast++;
		if (lan == 0) {		/* internal network */
			lipmxStats.InBroadcastInternal++;
			IpxRouteDataToSocket( mp);
		} else {
			lipmxStats.InBroadcastNIC++;
			/* Bridges accept broadcasts from external nets to
			** internal diag sock (NW DOS/C-interface pg. 7-5).
			*/
			if (inboundSocket == diagSocket) {
				lipmxStats.InBroadcastDiag++;
				if ((tmpMp = copymsg(mp)) != NULL) {
					lipmxStats.InBroadcastDiagFwd++;
					LipmxRoutePacket(q, tmpMp);
				}
				if (lipmxStats.IpxSocketMux) {
					IPXCOPYNET(&LipmxLans[0].ipxNet, ipxHeader->dest.net);
					lipmxStats.InBroadcastDiagRoute++;
					IpxRouteDataToSocket( mp);
				} else {
					lipmxStats.InBroadcastDiagResp++;
					LipmxIpxDiagnostics(mp,lan);
					/* Restore just in case altered by pullup */
					ipxHeader = (ipxHdr_t *)mp->b_rptr;
				}
			} else {
				NWSLOG((LIPMXID,0,PNW_DROP_PACKET,SL_TRACE,
					"LipmxDlUnitdataInd: Bcast to local Net, route it"));
				lipmxStats.InBroadcastRoute++;
				LipmxRoutePacket(q, mp);

			}
		}
	}
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxRoutePacket(queue_t *q, mblk_t *mp)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LipmxRoutePacket(queue_t *q, mblk_t *mp)
{	uint32		destinationNet; /* net Order */
	Lan_t		*lanInfo = (Lan_t *)q->q_ptr;
	uint32		outLan;
	mblk_t		*preMp;
	ipxHdr_t	*ipxHeader;
	pl_t		lvl;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LipmxRoutePacket"));
	if (lanInfo == NULL) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxRoutePacket: q->q_ptr is NULL"));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}
	/*
	** The message pointer should be checked for validity and size
	** before this call is made as it is not checked here before it
	** is dereferenced.
	*/
	ipxHeader = (ipxHdr_t *)mp->b_rptr;
	IPXCOPYNET(ipxHeader->dest.net, &destinationNet);

	/*
	** Have RR prepend LLI, fill in, so we just send.
	*/
	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	preMp = RROUTER.GetRouteInfo(ipxHeader, (void **)&outLan);
	ATOMIC_INT_DECR(&RRIPX.Ripcount);
	if (preMp != NULL) {
		preMp->b_cont = mp;
		mp = preMp;
	}
	/* outLan is set as a side-effect of RROUTER.GetRouteInfo */
	if (outLan >= ConfiguredLans) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxRoutePacket: Drop pkt: dest net 0x%X not in router table",
			GETINT32(destinationNet)));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	/*
	** If the packet is destined for the same lan it came from,
	** it is a bad packet and should be dropped.
	*/
	if (outLan == lanInfo->lan) {
		NWSLOG((LIPMXID,0,PNW_DROP_PACKET,SL_TRACE,
			"LipmxRoutePacket: Drop pkt whose srcNet == destNet."));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	/* We're crossing a router, so always increment hops */
	if (ipxHeader->pt != IPX_PROPAGATION_TYPE)
		++ipxHeader->tc;

	if (outLan == 0) {
		/*	Send to internal lan */
		/* Even if hops (ie; tc) > IpxMaxHops */
		IpxRouteDataToSocket(mp);
		NTR_VLEAVE();
		return;
	}	/* end route to internal lan */

	if (ipxHeader->tc > IpxMaxHops) {
		NWSLOG((LIPMXID,0,PNW_DROP_PACKET,SL_TRACE,
			"LipmxRoutePacket: Max hops exceeded, drop packet"));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}
	lvl = LOCK(LipmxLans[outLan].lanLock, plstr);
	if (LipmxLans[outLan].qbot) {
		if (canputnext(LipmxLans[outLan].qbot)) {
			ATOMIC_INT_INCR(&LipmxLans[outLan].qcount);
			UNLOCK(LipmxLans[outLan].lanLock, lvl);
			putnext(LipmxLans[outLan].qbot, mp);
			ATOMIC_INT_DECR(&LipmxLans[outLan].qcount);
		} else {
			UNLOCK(LipmxLans[outLan].lanLock, lvl);
			putq(LipmxLans[outLan].qbot, mp);
		}
	} else {
		UNLOCK(LipmxLans[outLan].lanLock, lvl);
		NWSLOG((LIPMXID,0,PNW_DROP_PACKET,SL_TRACE,
			"LipmxRoutePacket: Drop packet: no lower q"));
		freemsg(mp);
	}
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxDupBroadcastPacket(queue_t *q, mblk_t *mp)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LipmxDupBroadcastPacket(queue_t *q, mblk_t *mp)
{	uint32				lan;
	Lan_t				*localLan;
	ipxHdr_t			*ipxHeader;
	ipxHdr_t			*dupIpxHeader;
	uint8				*traversedNetPtr;
	uint32				net, traversedNet;
	int					i;
	mblk_t				*dupMp;
	uint16				len, pulluplen;
	void				*rrLanKey;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxDupBroadcastPacket"));

	ipxHeader = (ipxHdr_t *)mp->b_rptr;
	/*
	** The next if statement assumes that NO_ADD_SRC_ADDR is set
	** only on packets that are being routed from another network
	** and did not originate on this machine.
	*/
	/* NULL q means this came from the upstairs */
	if (q != NULL) {
		localLan = (Lan_t *)q->q_ptr;
		if (localLan == NULL) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxDupBroadcasePacket: q->q_ptr is NULL"));
			freemsg(mp);
			NTR_VLEAVE();
			return;
		}
		rrLanKey = localLan->rrLanKey;
		net = localLan->ipxNet;
	} else {
		/*
		** If ADD_SRC_ADDR then this packet came from the internal
		** network (0) and the net pointer can be extracted from
		** the LipmxLans structure.
		*/
		rrLanKey = LipmxLans[0].rrLanKey;
		net = LipmxLans[0].ipxNet;
	}
	pulluplen = IPX_PROPAGATION_LENGTH + IPX_HDR_SIZE;	/* Size required */
	len = PGETINT16(&ipxHeader->len);
	if ((rrLanKey == NULL)
			|| (ipxHeader->tc >= IPX_PROPAGATION_MAX_NETS)
			|| (len < pulluplen)) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxDupBroadcastPacket: Unable to broadcast interlan broadcast packet."));
		freemsg(mp);
		lipmxStats.InNoPropagate++;
		NTR_VLEAVE();
		return;
	}

	if ((uint16)DATA_SIZE(mp) < pulluplen) {
		if (pullupmsg(mp, pulluplen) == 0) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxDupBroadcastPacket: pullup of fragmented packet failed"));
			lipmxStats.InNoPropagate++;
			freemsg(mp);
			NTR_VLEAVE();
			return;
		}
		ipxHeader = (ipxHdr_t *)mp->b_rptr;
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxDupBroadcastPacket: pullup of fragmented packet succeeded, ipxHeader = %x",
			ipxHeader));
	}

	traversedNetPtr = (uint8 *)(mp->b_rptr + IPX_HDR_SIZE);
	for(i = 0; i < (int)ipxHeader->tc; i++) {
		IPXCOPYNET(traversedNetPtr, &traversedNet);
		traversedNetPtr += IPX_NET_SIZE;
		if (net == traversedNet) {
			/*
			** This packet has been on this net before!
			** Do not rebroadcast it on the same net.
			*/
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxDupBroadcastPacket: Ignoring type 20 packet re-broadcast over net 0x%X.",
				net));
			lipmxStats.InNoPropagate++;
			freemsg(mp);
			NTR_VLEAVE();
			return;
		}
	}
	NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
		"LipmxDupBroadcastPacket: Set net 0X%x in slot %d",
		net, i));

	/*
	** This packet should be duplicated. Get it ready.
	*/
	IPXCOPYNET(&net, traversedNetPtr);
	ipxHeader->tc += 1;

	for(lan = 1; lan < ConfiguredLans; lan++) {
		/* This should not be sent out slow links, but we currently
		** have no proper way of knowing/configuring what is a slow link
		*/
		traversedNetPtr = (uint8 *)(mp->b_rptr + IPX_HDR_SIZE);
		for(i = 0; i < (int)ipxHeader->tc; i++) {
			IPXCOPYNET(traversedNetPtr, &traversedNet);
			traversedNetPtr += IPX_NET_SIZE;
			if (LipmxLans[lan].ipxNet == traversedNet) {
				/*
				** This packet has been on this net before!
			 	** Do not rebroadcast it on the same net.
			 	*/
				NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
					"LipmxDupBroadcastPacket: NetBios Packet has been on lan %d net 0x%X before - skipping.",
					lan, traversedNet));
				goto CheckNextLan;
			}
		}
		/*
		** This packet has never been on this lan.
		** If I can copy it then send the copy out.
		*/
		if (!(dupMp = copymsg(mp))) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxDupBroadcastPacket: Unable to copymsg(mp = 0x%X).", mp));
			continue;
		}
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxDupBroadcastPacket: Sending NetBios Packet to lan %d net 0x%X",
			lan, LipmxLans[lan].ipxNet));
		dupIpxHeader = (ipxHdr_t *)dupMp->b_rptr;
		IPXCOPYNET(&LipmxLans[lan].ipxNet, dupIpxHeader->dest.net);
		LIPMXsendData(dupMp, NULL, 0);
		if (q != NULL)
			lipmxStats.InPropagation++;
		else
			lipmxStats.OutPropagation++;

	CheckNextLan:
		;
	}
	/*
	**	Send one copy of packet to internal net if we have a real one
	*/
	if (LipmxLans[0].ipxNet != LipmxLans[1].ipxNet) {
		IpxRouteDataToSocket( mp);
	} else {
		freemsg( mp);
	}
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIpxDiagnostics(mblk_t *mp, uint32 lan)
 *	IpxDiagnostics: Respond to valid Ipx Configuration Request packets
 *	addressed to socket 0x0456.
 *
 *	The request will be an ipxHdr, followed by an exclusion count and
 *	0 to ? node addresses. Do not respond if our node is listed.
 *
 *	The response will return the diagnostics version number, the
 *	Spx socket number to which Spx diagnostics requests should be
 *	addressed, how we are configured, and a list of lan card info.
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LipmxIpxDiagnostics(mblk_t *mp, uint32 lan)
{	int		i;
	int		lanBoards;	/* count of physical boards doing routing */
	mblk_t	*mpResp;
	uint8	*component;
	ipxHdr_t	*reqHeader, *respHeader;
	lanDriver_t		*lanDriver = 0;
	bridgeDrivers_t		*bridgeDrivers;
	ipxConfiguration_t	*config;
#ifdef NON_ROUTER
	uint16	 length;
	int8	 count;
	exclusionList_t		*excludeList;
	exclusionEntry_t	*exclusion;
#endif

	NTR_ENTER(2, mp, lan, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,"Enter LipmxIpxDiagnostics"));

	reqHeader = (ipxHdr_t *)mp->b_rptr;

#ifdef NON_ROUTER
	/*
	** Make sure the Data is big enough to hold the data received
	*/
	length = PGETINT16(reqHeader->len);
	if (DATA_SIZE(mp) < length) {
		if (pullupmsg(mp, length) == 0) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxIpxDiagnostics: pullup failed, drop pkt"));
			freemsg(mp);
			NTR_VLEAVE();
			return;
		}
		lipmxStats.InCoalesced++;
		reqHeader = (ipxHdr_t *)mp->b_rptr;
	}

	/*
	** The count for exclusion list and the list itself is only needed
	** if we are not a router.  The header is sufficient for the
	** response.  If lipmx is responding, we must be a router!
	**
	** Routers should always answer even if in the exclusion
	** list.  Therefore we ifdef this code out
	** Check exclusion addresses for server address.
	** If on list, go away.
	*/
	exclusion = (exclusionEntry_t *)((caddr_t)excludeList + 1);
	excludeList = (exclusionList_t *) ((caddr_t)reqHeader + IPX_HDR_SIZE);
	count = (int)excludeList->exclusionCount;
	if (len != (IPX_HDR_SIZE + 1 + (count * sizeof(IPX_NODE_SIZE)))) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxIpxDiagnostics: Drop pkt whose size does not match count = %xh, len = %xh, computed len = %xh",
			count, length, IPX_HDR_SIZE + 1 + (count * sizeof(IPX_NODE_SIZE))));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	while (count--) {
		if (IPXCMPNODE(exclusion, LipmxLans[lan].ipxNode)) {
			exclusion++;
			continue;
		} else {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxIpxDiagnostics: Excluded"));
			freemsg(mp);
			NTR_VLEAVE();
			return;
		}
	}
#endif

#ifdef DIAG_DELAY
	/* If the request is a broadcast, the reply should
	** be delayed (by time = f(last byte))
	** per "System Interface Technical Overview, sec. 6 pg. 5.
	** Otherwise, respond immediately
	*/
	save = reqHeader->dest.node[0];
	for (i = 1; i < 6 ; ++i)
		save &= reqHeader->dest.node[i];

	if (save != 0xff)
		;
	else
		;
#endif

	if (!(mpResp = allocb(sizeof(ipxConfigResponse_t), BPRI_MED))) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxIpxDiagnostics: allocb failed for config response, size = %xh",
			sizeof(ipxConfigResponse_t)));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}
	MTYPE(mpResp) = M_DATA;
	respHeader = (ipxHdr_t *)mpResp->b_rptr;
	/* best guess on these fields */
	PPUTINT16(IPX_CHKSUM, &respHeader->chksum);
	respHeader->tc = IPX_TRANSPORT_CONTROL;
	respHeader->pt = 0;

	/* fill in source of response packet */
	IPXCOPYNET(&LipmxLans[lan].ipxNet, respHeader->src.net);
	IPXCOPYNODE(&LipmxLans[lan].ipxNode, respHeader->src.node);
	PPUTINT16(diagSocket, respHeader->src.sock);

	/* fill in destination of response packet */
	IPXCOPYNET(reqHeader->src.net, respHeader->dest.net);
	IPXCOPYNODE(reqHeader->src.node, respHeader->dest.node);
	IPXCOPYSOCK(reqHeader->src.sock, respHeader->dest.sock);

	config = (ipxConfiguration_t *) ((caddr_t)respHeader
										+ sizeof(ipxHdr_t));
	config->majorVersion = DIAGS_VER_MAJ;
	config->minorVersion = DIAGS_VER_MIN;
	/* No internal net means no spx */
	config->spxDiagSock = 0;
	config->numberOfComponents = 0;

	/* We are never(?) a shell, shell driver, or VAP shell, so we
	** must be a bridge driver (unless we're non-dedicated ?).
	*/
	component = &config->componentStructure[0];
	*component = BRIDGE_DRIVER_COMPONENT;
	++component;
	++(config->numberOfComponents);

	/* We must do Internal and or External bridging.
	** Usually it is assumed that an Internal Bridge is a
	** file server (not true for NWU). Are we non-dedicated ?
	** Can we do both on mutually exclusive nets ?
	*/

	/* NULL so we can later test Internal vs Ext. */

	*component = 0;
	for (i = 1, lanBoards = 0; i < ConfiguredLans; i++) {
		/* Valid netPtr means this lan is registered
		** in the routing table.
		*/
		if (LipmxLans[i].rrLanKey != NULL) {
			++lanBoards;
				/* NOT! lan 0 can be routed to - we do Internal */
				/* Real confusion here if this is supposed to
	 			** indicate FILE_SERVER or INTERNAL_BRIDGE. The
	 			** latter is easier, so guess what we do?
				**		*component = INTERNAL_BRIDGE;
	 			*/

			/* multiple registered lanBoards and active
			** routing means we're an external bridge,
			** so tell the requester.
			*/
			if (lanBoards > 1) {
				/* If we're not an Internal Bridge (which seems
				** to subsume External Bridging as far as
				** diagnostics is concerned), let requester
				** know we do external routing. If type == INTERNAL,
				** does this assume EXTERNAL ?
				*/
				if (!(*component))
					*component = EXTERNAL_BRIDGE;
				break;
			}
		}
	}

	/* If we're doing anything useful, report what lans
	** we are doing it on.
	*/
	if (*component) {
		++component;
		++(config->numberOfComponents);

		bridgeDrivers = (bridgeDrivers_t *)component;
		bridgeDrivers->numberOfNets = 0;
		lanDriver = bridgeDrivers->drivers;
		for(i = 1; i < ConfiguredLans; i++) {
			if (LipmxLans[i].rrLanKey != NULL) {
				++(bridgeDrivers->numberOfNets);
				lanDriver->localNetworkType = LAN_BOARD;
				IPXCOPYNET(&LipmxLans[i].ipxNet, lanDriver->network);
				IPXCOPYNODE(LipmxLans[i].ipxNode, lanDriver->node);
				++lanDriver;
			}
		}
		component = (uint8 *)lanDriver;
	}

	mpResp->b_wptr = component;
	PPUTINT16(DATA_SIZE(mpResp), &respHeader->len);

#ifdef HARDDEBUG
	/*
	 *+ Ipx Diagnostics debug
	 */
	cmn_err(CE_NOTE,
		"LipmxIpxDiagnostics: Hdr @ 0x%x\nconfig @ 0x%x\ncomponent @ 0x%x\nbridges @ 0x%x\ncards @ 0x%x",
		respHeader, config, component, bridgeDrivers, lanDriver);
	/*
	 *+ Ditto
	 */
	cmn_err(CE_NOTE,
		"LipmxIpxDiagnostics: configResp Packet size %d:", respHeader->len);
	for(i=0; i<160; i++)
		/*
		 *+ Ditto
		 */
		 cmn_err(CE_CONT, "LipmxIpxDiagnostics: %x", *(mpResp->b_rptr + i));
#endif
	freemsg(mp);
	NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
		"LipmxIpxDiagnostics: ConfigResponse packet size = %xh",
		DATA_SIZE(mpResp)));
	LIPMXsendData(mpResp, NULL, 0);
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxInitLocalNets(void)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LipmxInitLocalNets(void)
{	uint32	i;
	mblk_t	*mp;
	pl_t	lvl;
	int		count;

	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxInitLocalNets: LipmxLans = 0x%X", LipmxLans));
	if (LipmxLans) {
		for(i=0; i< ConfiguredLans; i++) {
			lvl = LOCK(LipmxLans[i].lanLock, plstr);
			while (LipmxLans[i].bcastPaced) {
				mp = LIPMXTakeItOff( &LipmxLans[i].bcastPaced);
				freemsg(mp);
			}
			while (LipmxLans[i].directedPaced) {
				mp = LIPMXTakeItOff( &LipmxLans[i].directedPaced);
				freemsg(mp);
			}
			UNLOCK(LipmxLans[i].lanLock, lvl);
			/* 
			** timeout for sendId calls "qenable(q)", we are already
			** unlinked, our service routine will not run again to 
			** call timeout().  Just need to untimeout if a timer is
			** active.
			** Since timout calls qenable we can hold the lock
			** across untimeout/
			*/ 
			count = 10;
			while (count) {
				count--;
				if (LipmxLans[i].sendId) {
					untimeout(LipmxLans[i].sendId);
					LipmxLans[i].sendId = 0;
					count = 10;
				}
				drv_usecwait(5);
			}
			LOCK_DEALLOC(LipmxLans[i].lanLock);
		}
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxInitLocalNets: KMEM: Free lan structure at 0x%X, size %d", 
				LipmxLans, sizeof( ((int)ConfiguredLans * sizeof(Lan_t)))));
		kmem_free(LipmxLans, ((int)ConfiguredLans * sizeof(Lan_t)));
		LipmxLans = NULL;
	}
	ConfiguredLans = 0;
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxRouteRIP(int flag)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LipmxRouteRIP(int flag)
{	rrLanData_t	 lanData;

	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxRouteRIP"));
	RouteRipToIpx = flag;
	if (RouteRipToIpx == ENABLE_RIP) {
		lanData.ripCfg.actions
			= LipmxLans[0].ripSapInfo.rip.actions | SEND_RIPS;
		++RouteRipToIpx;
	} else {
		lanData.ripCfg.actions
			= LipmxLans[0].ripSapInfo.rip.actions & ~SEND_RIPS;
		RouteRipToIpx = 0;
	}
	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	RROUTER.UpdateLanKey(LipmxLans[0].rrLanKey, &lanData);
	ATOMIC_INT_DECR(&RRIPX.Ripcount);
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxSetUnderIpx(void)
 *	If the socket multiplexor is configured in, it will
 *	call this function to let us know it is there.
 *	We can therefore know whether or not to call IpxRouteDataToSocket
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LipmxSetUnderIpx(void)
{
	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxSetUnderIpx"));
	lipmxStats.IpxSocketMux = 1;
	NTR_VLEAVE();
	return;
}

/*
 * int LipmxGetMinMaxSDU(void)
 *	Get the minimum SDU of all lans maxSDUs.
 *
 * Calling/Exit State:
 *	
 *	
 */
int
LipmxGetMinMaxSDU(void)
{
	uint32	lan;
	int		minmaxSDU = 0xffff;

	NTR_ENTER(0, 0, 0,0,0,0);
	if (!ConfiguredLans) {
		minmaxSDU = IPX_MAX_PACKET_DATA;
		return(NTR_LEAVE(minmaxSDU));
	}
	for(lan = 0; lan < ConfiguredLans; lan++) {
		if (LipmxLans[lan].dlInfo.SDU_max < minmaxSDU) 
			minmaxSDU = LipmxLans[lan].dlInfo.SDU_max;
	}
	minmaxSDU -= IPX_HDR_SIZE;

	/* if streams size is not unlimited and is less than minmaxSDU */
	if ((strmsgsz != 0) && (strmsgsz < minmaxSDU))
		minmaxSDU = strmsgsz;

	/*
	 * IPX packets must be an even number of bytes, if minmaxSDU is odd 
	 * decrement it.
	 */
	if ( minmaxSDU & (int)0x01 ) {
		minmaxSDU--;
	}


	return(NTR_LEAVE(minmaxSDU));
}

/*
 * int LipmxGetMaxSDU(uint32 net)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
int
LipmxGetMaxSDU(uint32 net)
{	uint32   lan;
	int	maxSDU = 0;

	NTR_ENTER(1, net, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxGetMaxSDU: net 0x%X", GETINT32(net)));
	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	if (RROUTER.MapNetToIpxLanKey(net, (void **)&lan))
		maxSDU = LipmxLans[lan].dlInfo.SDU_max;
	ATOMIC_INT_DECR(&RRIPX.Ripcount);
	return(NTR_LEAVE(maxSDU));
}

/*
 * uint32 LipmxGetConfiguredLans(void)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
uint32
LipmxGetConfiguredLans(void)
{
	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxGetConfiguredLans"));
	return((uint32)NTR_LEAVE(ConfiguredLans));
}

/*
 * void LipmxSetMaxHops(uint16 maxHops)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LipmxSetMaxHops(uint16 maxHops)
{
	NTR_ENTER(1, maxHops,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxSetMaxHops"));
	IpxMaxHops = maxHops;
	if (maxHops < 2)					/* minimum setting */
		maxHops = IPX_ROUTER_DOWN;	/* default 127 */
	IpxMaxHops = maxHops;
	NTR_VLEAVE();
	return;
}

/*
 * uint16 LipmxGetMaxHops(void)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
uint16
LipmxGetMaxHops(void)
{
	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxGetMaxHops"));
	return((uint16)NTR_LEAVE(IpxMaxHops));
}

/*
 * void LipmxInitLanData(uint32 lan)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LipmxInitLanData(uint32 lan)
{
	pl_t	lvl;

	NTR_ENTER(1, lan, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxInitLanData"));

	ASSERT(lan < ConfiguredLans);
	LipmxLans[lan].lan = lan;
	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	LipmxLans[lan].rrLanKey
		= RROUTER.InvalidateLanKey(LipmxLans[lan].rrLanKey);
	ATOMIC_INT_DECR(&RRIPX.Ripcount);

	lvl = LOCK(LipmxLans[lan].lanLock, plstr);
	LipmxLans[lan].ipxNet = (uint32)-1;
	bzero((char *)LipmxLans[lan].ipxNode,IPX_NODE_SIZE);
	LipmxLans[lan].state = IPX_UNBOUND;
	LipmxLans[lan].qbot = NULL;
	UNLOCK(LipmxLans[lan].lanLock, lvl);

	NTR_VLEAVE();
	return;
}

/*
 * int LipmxSetConfiguredLans(uint32 lans)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
int
LipmxSetConfiguredLans(uint32 lans)
{	Lan_t   *localNets;
	int		i;

	NTR_ENTER(1, lans, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxSetConfiguredLans: set to %d", lans));
	if (ConfiguredLans) {
	   NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
		"LipmxSetConfiguredLans: AlreadySet"));
#ifdef DEBUG
		/*
		 *+ Set configured lans called twice
		 */
		cmn_err(CE_WARN, "LipmxSetConfiguredLans: Already set");
#endif /* DEBUG */
		return(NTR_LEAVE(0));
	}
	if ((lans < IPX_MIN_CONFIGURED_LANS)
		|| (lans > IPX_MAX_CONFIGURED_LANS)) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxSetConfiguredLans: Too few/many Lans"));
#ifdef DEBUG
		/*
		 *+ Either no lans configured or too many lans configured
		 */
		cmn_err(CE_WARN,"LipmxSetConfiguredLans: Too few/many Lans");
#endif /* DEBUG */
		return(NTR_LEAVE(0));
	}

	if (!(localNets = (Lan_t *)
					kmem_alloc((int)lans * sizeof(Lan_t), KM_NOSLEEP))) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxSetConfiguredLans: 'kmem_alloc failed"));
#ifdef DEBUG
		/*
		 *+ kmem_alloc failed
		 */
		cmn_err(CE_WARN,"LipmxSetConfiguredLans: kmem_alloc failed");
#endif /* DEBUG */
		return(NTR_LEAVE(0));
	}
	NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
		"LipmxSetConfiguredLans: KMEM: Allocate Lan structure at 0x%X, size %d",
			localNets, sizeof((int)lans * sizeof(Lan_t))));
	bzero( (caddr_t)localNets, (int)lans * sizeof(Lan_t));
	for( i = 0; i < lans; i++) {
		localNets[i].lanLock = 
			LOCK_ALLOC(LAN_LOCK, plstr, &lipmxLanLkinfo, KM_NOSLEEP);
		ATOMIC_INT_INIT(&localNets[i].qcount, 0);
		if (localNets[i].lanLock == NULL) {
			/* no memory for the lock*/
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
					"LipmxSetConfiguredLans: Can't ALLOC LOCK"));
#ifdef DEBUG
		/*
		 *+ Lock allocation failed
		 */
		cmn_err(CE_WARN,"LipmxSetConfiguredLans: Can't ALLOC LOCK");
#endif /* DEBUG */
			for(i = 0; i < lans; i++) {
				/*release any locks ALLOCed*/
				if (localNets[i].lanLock)
					LOCK_DEALLOC(localNets[i].lanLock);
			}
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxSetConfiguredLans: KMEM: Free Lan structure at 0x%X, size %d", 
					localNets, sizeof(((int)lans * sizeof(Lan_t)))));
			kmem_free(localNets, ((int)lans * sizeof(Lan_t)));
			return(NTR_LEAVE(0));
		}
	}

	NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"LipmxSetConfiguredLans: = %x",lans));

	LipmxLans = localNets;
	ConfiguredLans = lans;
	for(lans = 0; lans < ConfiguredLans; lans++)
		LipmxInitLanData(lans);
	return(NTR_LEAVE(1));
}

/*
 * int LipmxLinkLan(struct linkblk *linkp)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
int
LipmxLinkLan(struct linkblk *linkp)
{	uint32	lan;
	pl_t	lvl;

	NTR_ENTER(1, linkp, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxLinkLan"));
	/*
	** Find an unused LipmxLans slot in the array of private data
	** structures.  If they are all used up return and negative
	** acknowledgement.
	*/
	for(lan = 1; lan < ConfiguredLans; lan++) {
		lvl = LOCK(LipmxLans[lan].lanLock, plstr);
		if (LipmxLans[lan].qbot == NULL)
			break;
		UNLOCK(LipmxLans[lan].lanLock, lvl);
	}
	/*
	** After for loop exit  Lan_t is lock if lan < ConfiguredLans.
	** if lan >= ConfiguredLans no locks are set.
	*/
	if (lan >= ConfiguredLans) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxLinkLan: Attempt to link. Exceeded max lans %xh",lan));
		return(NTR_LEAVE(0));
	}
	/*
	** Set the bottom queue pointer, and change the state to linked.  
	** Also save the l_index for use by the set net ioctl that must follow.
	*/
	LipmxLans[lan].qbot = linkp->l_qbot;
	LipmxLans[lan].l_index = linkp->l_index;
	LipmxLans[lan].state = IPX_LINKED;
	NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"LipmxLinkLan: %xh linked to bottom queue: %xh",lan,
		LipmxLans[lan].qbot));
	/*
	** Set the lower read and write queues q_ptr to point to the
	** private data structure for this lan.
	*/
	(LipmxLans[lan].qbot)->q_ptr = (caddr_t) &LipmxLans[lan];
	RD(LipmxLans[lan].qbot)->q_ptr = (caddr_t) &LipmxLans[lan];
	UNLOCK(LipmxLans[lan].lanLock, lvl);
	return(NTR_LEAVE(1));
}

/*
 * int LipmxUnlinkLan(int index)
 *	Called as a result of an M_IOCTL/I_UNLINK
 *
 * Calling/Exit State:
 *	
 *	
 */
int
LipmxUnlinkLan(int index)
{	uint32	lan;
	int		count = 0;
	void	*rrLanKey;
	pl_t	lvl;
	mblk_t *mp;
	int i,j;

	NTR_ENTER(1, index, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxUnlinkLan"));

	for(lan=0; lan < ConfiguredLans; lan++) {
		if ((index == -1) || (LipmxLans[lan].l_index == index)) {
			NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
				"LipmxUnlinkLan: l_index 0x%X matches Lan[%xh] bottom queue: %xh",
				index, lan, LipmxLans[lan].qbot));
			NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
				"LipmxUnlinkLan: Invalidate lan 0x%X", lan));
			ATOMIC_INT_INCR(&RRIPX.Ripcount);
			rrLanKey = RROUTER.InvalidateLanKey(LipmxLans[lan].rrLanKey);
			ATOMIC_INT_DECR(&RRIPX.Ripcount);
		}
	}
	/*
	 * delay a bit, allow ripx service routine time to run
	 */
	drv_usecwait(5);

	for(lan=0; lan < ConfiguredLans; lan++) {
		if ((index == -1) || (LipmxLans[lan].l_index == index)) {

			/*
			 *	Clear out all paced packets
			 */
			lvl = LOCK(LipmxLans[lan].lanLock, plstr);
			while (LipmxLans[lan].bcastPaced) {
				mp = LIPMXTakeItOff( &LipmxLans[lan].bcastPaced);
				freemsg(mp);
			}
			while (LipmxLans[lan].directedPaced) {
				mp = LIPMXTakeItOff( &LipmxLans[lan].directedPaced);
				freemsg(mp);
			}
			UNLOCK(LipmxLans[lan].lanLock, lvl);

			/*
			 *	Turn off paced packet timer
			 */
			if (LipmxLans[lan].sendId) {
				/* 
				** timeout for sendId calls "qenable(q)", we are already
				** unlinked, our service routine will not run again to 
				** call timeout().  Just need to untimeout if a timer is
				** active.
				** Since timout calls qenable we can hold the lock
				** across untimeout/
				*/ 
				NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
					"LipmxUnlinkLan: untimeout paced packet timer, lan %d", lan));
				untimeout(LipmxLans[lan].sendId);
				LipmxLans[lan].sendId = (toid_t)0;
			}

			NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
				"LipmxUnlinkLan: wait for lan 0x%X lower queue to empty, count = %d",
				lan, qsize( LipmxLans[lan].qbot)));

			/* 
			 * wait until all inprogress packets are sent
			 */
			j = 0;
			while( qsize( LipmxLans[lan].qbot) || 
					ATOMIC_INT_READ(&LipmxLans[lan].qcount)) {
			    i = qsize( LipmxLans[lan].qbot);
				drv_usecwait(50);
				if( ( i > 0) && (i == qsize( LipmxLans[lan].qbot))) {
					NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
						"LipmxUnlinkLan: Svc not running, count %d unchanged",
						qsize( LipmxLans[lan].qbot)));
					/* Force service routine to run */
					lipmxlwsrv(LipmxLans[lan].qbot);
					/*
					**	Don't try this too many times, just give up
					*/
					if( j++ >= 10) {
						NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
							"LipmxUnlinkLan: Retry exceeded, give up remaing count %d", qsize( LipmxLans[lan].qbot)));
						break;
					}
				}
			}
			
			lvl = LOCK(LipmxLans[lan].lanLock, plstr);
			flushq( LipmxLans[lan].qbot, FLUSHALL);
			(LipmxLans[lan].qbot)->q_ptr = (caddr_t) NULL;
			RD(LipmxLans[lan].qbot)->q_ptr = (caddr_t) NULL;
			LipmxLans[lan].rrLanKey = rrLanKey;
			LipmxLans[lan].l_index = 0;
			LipmxLans[lan].qbot = NULL;
			LipmxLans[lan].state = IPX_UNBOUND;
			LipmxLans[lan].ipxNet = (uint32)-1;
			bzero((char *)LipmxLans[lan].ipxNode,IPX_NODE_SIZE);
			count++;
			UNLOCK(LipmxLans[lan].lanLock, lvl);
		}
	}
#ifdef DEBUG
	if ((count > 1) && (index != -1)) {
		/*
		 *+ Bad -- we have some kind of internal logic problem
		 */
		cmn_err(CE_PANIC,
			"UnlinkLan: Way Funky - multiple lans had same l_index !!");
		return(NTR_LEAVE(-1));
	}
#endif /* DEBUG */
	count = 0;
	for (lan=0; lan < ConfiguredLans; lan++) {
		if (LipmxLans[lan].l_index != 0) {
			count++;
		}
	}
	return(NTR_LEAVE(count));	/* Return count of lans left */
}

/*
 * int LipmxGetLanInfo(lanInfo_t *lanInfo)
 *	Called as a result of an M_IOCTL/IPX_GET_LAN_INFO
 *
 * Calling/Exit State:
 *	
 *	
 */
int
LipmxGetLanInfo(lanInfo_t *lanInfo)
{	uint32	lan;
	pl_t	lvl;

	NTR_ENTER(1, lanInfo, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxGetLanInfo: for lan[%d]", lanInfo->lan));

	if ((lan = lanInfo->lan) >= ConfiguredLans)
		return(NTR_LEAVE(0));

	lvl = LOCK(LipmxLans[lan].lanLock, plstr);
	/*
	** No Internal Network and Lan 0 request, pwd
	*/
	if ((lan == 0) && (lipmxStats.IpxInternalNet == 0)) {
		bzero( (char *)lanInfo, sizeof( lanInfo_t));
	} else {
		lanInfo->lan = LipmxLans[lan].lan;
		lanInfo->state = LipmxLans[lan].state;
		lanInfo->network = GETINT32(LipmxLans[lan].ipxNet);
		lanInfo->muxId = (uint32)LipmxLans[lan].l_index;
		IPXCOPYNODE(LipmxLans[lan].ipxNode, lanInfo->nodeAddress);
		bcopy((char *)&LipmxLans[lan].dlInfo,
				(char *)&lanInfo->dlInfo, sizeof(dlInfo_t));
		bcopy((char *)&LipmxLans[lan].ripSapInfo,
				(char *)&lanInfo->ripSapInfo, sizeof(ripSapInfo_t));
	}
	UNLOCK(LipmxLans[lan].lanLock, lvl);
	return(NTR_LEAVE(1));
}

/*
 * int LipmxSetLanInfo(lanInfo_t *lanInfo)
 *	Called as a result of an M_IOCTL/IPX_SET_LAN_INFO
 *
 * Calling/Exit State:
 *	
 *	
 */
int
LipmxSetLanInfo(lanInfo_t *lanInfo)
{	int			muxId;
	uint32		lan;
	uint32		ipxNetAddr;
	uint8		*physAddrPtr;
	dlInfo_t 	*dlInfo;
	pl_t		lvl;

	NTR_ENTER(1, lanInfo, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LipmxSetLanInfo: lanInfo_t *=0x%X", lanInfo));

	ipxNetAddr = GETINT32(lanInfo->network);
	NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"LipmxIocSetLanInfo: network address: machine order: %xh",
		GETINT32(ipxNetAddr)));

	/* check for illegal network numbers */
	if ((ipxNetAddr == 0) || (ipxNetAddr == (uint32)-1)) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxSetLanInfo: Network number 0x%X is illegal",
				GETINT32(ipxNetAddr)));
		return(NTR_LEAVE(0));
	}

	dlInfo = &lanInfo->dlInfo;
	physAddrPtr = lanInfo->dlInfo.dlAddr + dlInfo->physAddrOffset;
	if (((physAddrPtr + dlInfo->ADDR_length)
				> ((uint8 *)dlInfo + sizeof(dlInfo_t)))
			|| (physAddrPtr < (uint8 *)dlInfo)) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxSetLanInfo: bad physAddrOffset = 0x%X",
			dlInfo->physAddrOffset));
#ifdef DEBUG
		/*
		 *+ Set Lan Info, app sent bad phyAddrOffset
		 */
		cmn_err(CE_PANIC,
			"LipmxSetLanINfo: bad physAddrOffset: dlAddr = 0x%X, dlInfo = 0x%X physAddrOffset = 0x%X",
			 lanInfo->dlInfo.dlAddr, dlInfo, dlInfo->physAddrOffset);
#endif
		return(NTR_LEAVE(0));
	}
	muxId = lanInfo->muxId;
	if (muxId == 0) {
		lan = 0;	/* This is the internal network */
		if (!LipmxLans) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxSetLanInfo: internal Lan and no LipmxLan array"));
			return(NTR_LEAVE(0));
		}
		/* make exported values available */
		if ((dlInfo->ADDR_length + IPX_NODE_SIZE)
				> sizeof(dlInfo->dlAddr)) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxSetLanInfo: bad size dlAddr length"));
			return(NTR_LEAVE(0));
		}
		lvl = LOCK(LipmxLans[lan].lanLock, plstr);
		LipmxLans[lan].l_index = 0;
		LipmxLans[lan].qbot = NULL;
		LipmxLans[lan].state = IPX_UNBOUND; /* 0 */
	} else {
		if (!LipmxLans) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxSetLanInfo: No LipmxLan array"));
			return(NTR_LEAVE(0));
		}
		for(lan = 1; lan < ConfiguredLans; lan++) {
			lvl = LOCK(LipmxLans[lan].lanLock, plstr);
			if (muxId == LipmxLans[lan].l_index)
				break;
			UNLOCK(LipmxLans[lan].lanLock, lvl);
		}
		/*
		** After for loop exits Lan_t is lock if lan < ConfiguredLans.
		** if lan >= ConfiguredLans no locks are set.
		*/
		if (lan >= ConfiguredLans){
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,"lan > max %xh",lan));
			return(NTR_LEAVE(0));
		}
		/*
	 	** Check the state to see if a lower stream has been bound to
	 	** this local network.  If not nak the ioctl.
	 	*/
		if (LipmxLans[lan].state == IPX_UNBOUND) {
			UNLOCK(LipmxLans[lan].lanLock, lvl);
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxSetLanInfo: lan %xh out of state %xh",
				lan,LipmxLans[lan].state));
			return(NTR_LEAVE(0));
		}
		LipmxLans[lan].state = IPX_IDLE;

	}
	IPXCOPYNODE(physAddrPtr, LipmxLans[lan].ipxNode);
#ifdef DLPI_DEBUG
	/*
	 *+ DLPI debug, display dlinfo
	 */
	cmn_err(CE_NOTE,"LipmxSetLanInfo: lan = %d, dlInfo = 0x%X)",
		lan, dlInfo);
	{	int i;
		/*
		 *+ Ditto
		 */
		cmn_err(CE_NOTE,"Lan[%d] node = ", lan);
		for(i = 0; i < 6; i++) {
			/*
			 *+ Ditto
			 */
			cmn_err(CE_CONT,"%X ", LipmxLans[lan].ipxNode[i]);
		}
		/*
		 *+ Ditto
		 */
		cmn_err(CE_NOTE,"");
	}
#endif /* DLPI_DEBUG */
	if( lanInfo->ripSapInfo.lanSpeed == 0) {
		lanInfo->ripSapInfo.lanSpeed = 1;	/* Don't allow divide by zero */
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxSetLanInfo: lanSpeed changed from zero to 1"));
	}
	bcopy((char *)&lanInfo->ripSapInfo,
		(char *)&LipmxLans[lan].ripSapInfo, sizeof(ripSapInfo_t));

	/* Now change physAddrOffset to refer to dlMp, not dlAddr */
	dlInfo->physAddrOffset += sizeof(dl_unitdata_req_t);

	bcopy((char *)dlInfo,
			(char *)&LipmxLans[lan].dlInfo,
			sizeof(dlInfo_t));
			/* too much size here
				- we only need SDUs and physAddrOffset */

	LipmxLans[lan].ipxNet = ipxNetAddr;

	/*
	** lan0 should have !DO_RIP until RIP socket is opened
	** lanData.rip.cfg.actions
	** = LipmxLans[lan].ripSapInfo.rip.actions;
	*/
	/*
	**	Inform IPX of internal net and node
	*/
	/* change ms to ticks  - round up */
	LipmxLans[lan].paceTicks
		= ((unsigned)LipmxLans[lan].ripSapInfo.rip.interPktGap
				* HZ + 999) / 1000;
	if ((LipmxLans[lan].paceTicks == 0)
			&& (LipmxLans[lan].ripSapInfo.rip.interPktGap != 0))
		LipmxLans[lan].paceTicks = 1;

	/* No more Internal, Shield IPX from this
	*/
	if (lan == 0) {
		IpxSetInternalNetNode(&ipxNetAddr, physAddrPtr);
		if (! IPXCMPNET( &ipxNetAddr, &LipmxLans[1].ipxNet)) {
			lipmxStats.IpxInternalNet = 1;		/* 1 = Internal Network exists */
			LipmxBuildRRlanKey(lan, lvl);
		} else {
			if (( ConfiguredLans > 2) || (! IPXCMPNODE(&LipmxLans[0].ipxNode,
					&LipmxLans[1].ipxNode))) {
				LipmxLans[lan].ipxNet = 0;
				UNLOCK(LipmxLans[lan].lanLock, lvl);
				return(NTR_LEAVE(0));
			}
			/*
			** No Internal Network, do not register with RIP
			*/
			lipmxStats.IpxInternalNet = 0;		/* 0 = NO Internal Network */
			UNLOCK(LipmxLans[lan].lanLock, lvl);
			return(NTR_LEAVE(1));
		}
	} else {
		LipmxBuildRRlanKey(lan, lvl);
	}

#ifdef RR_DEBUG
	/*
	 *+ DEBUG, display lan key
	 */
	cmn_err(CE_NOTE, "LipmxSetLanInfo: got lan %d key 0x%X",
		lan,LipmxLans[lan].rrLanKey);
	/*
	 *+ Ditto
	 */
	cmn_err(CE_NOTE, "EXIT LipmxSetLanInfo");
#endif /* RR_DEBUG */
	return(NTR_LEAVE(1));
}

/*
 * int LipmxBuildRRlanKey(uint32 lan, pl_t lvl)
 *	Lan_t lock must be held on entry. Release lock before returning.
 *
 * Calling/Exit State:
 *	
 *	
 */
int
LipmxBuildRRlanKey(uint32 lan, pl_t lvl)
{	rrLanData_t		lanData;

	NTR_ENTER(2, lan, lvl,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LipmxBuildRRlanKey: lan = %d", lan));

	if (LipmxLans[lan].ipxNet == -1) {
		UNLOCK(LipmxLans[lan].lanLock, lvl);
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"lan %xh doesn't have valid data", lan));
		return(NTR_LEAVE(0));
	}
	if (!LipmxGetDlMp(lan, &(lanData.prepend))) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"lan %xh can't get dlMp", lan));
#ifdef DEBUG
		/*
		 *+ Error, can't set dlMp
		 */
		cmn_err(CE_WARN,"LipmxSetLanInfo: ABORT can't set dlMp");
#endif /* DEBUG */
		UNLOCK(LipmxLans[lan].lanLock, lvl);
		LipmxInitLanData(lan);
		return(NTR_LEAVE(0));
	}
	/* RROUTER now becomes caretaker of dlMp */
	lanData.addrOffset = LipmxLans[lan].dlInfo.physAddrOffset;

	lanData.ipxNetAddr = LipmxLans[lan].ipxNet;
	lanData.ipxLanKey = (void *)(LipmxLans[lan].lan);
	IPXCOPYNODE(LipmxLans[lan].ipxNode, &lanData.ipxLanNode);
	/*
	** convert lanSpeed to ticks/576 byte packet per RIP spec
	** lanTicks = (0xFFFF ticks / (60 * 60) sec) * (10 (?) bits / byte)
	** 				* 576 bytes / (1 sec / (lanSpeed * 1000) bits)
	*/
	lanData.lanTicks = 10 * 576 * (uint32)0xFFFF / 60 / 60 / 1000
					 / (uint32)LipmxLans[lan].ripSapInfo.lanSpeed;
	if (lanData.lanTicks == 0)
		lanData.lanTicks = 1;
	/* key off lanTicks to get WAN_BIT or RELIABLE_BIT */
	lanData.lanType = 0;

	lanData.ripCfg.bcastInterval
			= LipmxLans[lan].ripSapInfo.rip.bcastInterval;
	lanData.ripCfg.ageOutIntervals
			= LipmxLans[lan].ripSapInfo.rip.ageOutIntervals;
	lanData.ripCfg.maxPktSize
			= LipmxLans[lan].ripSapInfo.rip.maxPktSize;
	lanData.ripCfg.actions
			= LipmxLans[lan].ripSapInfo.rip.actions;
	/* Get rid of lanData.ripLanInfo.interPktGap !!! */
	lanData.ripCfg.interPktGap
			= LipmxLans[lan].ripSapInfo.rip.interPktGap;

	UNLOCK(LipmxLans[lan].lanLock, lvl);
	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	LipmxLans[lan].rrLanKey = RROUTER.GrantLanKey(&lanData);
	ATOMIC_INT_DECR(&RRIPX.Ripcount);
	if (LipmxLans[lan].rrLanKey == NULL) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"lan %xh can't get dlMp", lan));
#ifdef DEBUG
		/*
		 *+ Can't set dlmp
		 */
		cmn_err(CE_WARN,"LipmxSetLanInfo: ABORT can't set dlMp");
#endif /* DEBUG */
		LipmxInitLanData(lan);
		return(NTR_LEAVE(0));
	}
	return(NTR_LEAVE(1));
}

/*
 * int LipmxGetDlMp(uint32 lan, mblk_t **dlMpp)
 *	Serialization should be guaranteed here.  Called as part of
 *	linkup, which is protected against concurrency by the fact
 *	that links happen from the control device of IPX, which
 *	is only getting one IOCTL at a time from the stream head.
 *
 * Calling/Exit State:
 *	Locks may be held across calls to this function.
 *	
 *	
 */
int
LipmxGetDlMp(uint32 lan, mblk_t **dlMpp)
{ 	int		 dlSize, cCode = 0;
	dl_unitdata_req_t *dlReq;
	dlInfo_t	*dlInfo = (dlInfo_t *)(&LipmxLans[lan].dlInfo);

	NTR_ENTER(2, lan, dlInfo, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxGetDlMp: lan %d", lan));
	if (lan) {
		dlSize = sizeof(dl_unitdata_req_t) + dlInfo->ADDR_length;

		if (!(*dlMpp = allocb(dlSize, BPRI_MED))) {
			NWSLOG((LIPMXID, 0, PNW_ERROR, SL_TRACE,
				"LipmxGetDlMp: couldnt alloc pre-formatted header "));
#ifdef DEBUG
			/*
			 *+ Alloc failed of dlMp
			 */
			cmn_err(CE_WARN, "LipmxGetDlMp: BOGUS ! ALLOC FAIL !");
#endif /* DEBUG */
			return(NTR_LEAVE(cCode));
		}

		dlReq = (dl_unitdata_req_t *)((*dlMpp)->b_rptr);
		(*dlMpp)->b_wptr = (uint8 *)dlReq + dlSize;
		/* physAddrOffset has been set to point past
		** dl_ud_req in SetLanInfo
		*/
		if ((dlInfo->physAddrOffset
				+ IPX_NODE_SIZE) > DATA_SIZE(*dlMpp)) {
#ifdef DEBUG
			/*
			 *+ Dlmp bad size
			 */
			cmn_err(CE_PANIC, "LipmxGetDlMp: BOGUS ! BAD SIZE !");
#endif /* DEBUG */
			NWSLOG((LIPMXID, 0, PNW_ERROR, SL_TRACE,
				"LipmxGetDlMp: mis-alignment of physAddrOffset"));
			freemsg((*dlMpp));
			*dlMpp = NULL;
			return(NTR_LEAVE(cCode));
		}
		dlReq->dl_dest_addr_offset = sizeof(dl_unitdata_req_t);
		bcopy((char *)dlInfo->dlAddr,
			(char *)dlReq + dlReq->dl_dest_addr_offset,
			dlInfo->ADDR_length);
		dlReq->dl_priority.dl_min = 0;
		dlReq->dl_priority.dl_max = 0;
		dlReq->dl_primitive = DL_UNITDATA_REQ;
		dlReq->dl_dest_addr_length = dlInfo->ADDR_length;
		MTYPE(*dlMpp) = M_PROTO;
		cCode = 1;
#ifdef DLPI_DEBUG
		/*
		 *+ DEBUG - display dlmp
		 */
		cmn_err(CE_NOTE,"LipmxGetDlMp");
		/*
		 *+ Ditto
		 */
		cmn_err(CE_NOTE,"  dlSize = %d", dlSize);
		/*
		 *+ Ditto
		 */
		cmn_err(CE_NOTE,"  ADDR_length = %d", dlInfo->ADDR_length);
		/*
		 *+ Ditto
		 */
		cmn_err(CE_NOTE,"  sizeof(dl_unitdata_req_t) = %d",
				sizeof(dl_unitdata_req_t));
		/*
		 *+ Ditto
		 */
		cmn_err(CE_NOTE,"  physAddrOffset = %d",
				dlInfo->physAddrOffset);
		/*
		 *+ Ditto
		 */
		cmn_err(CE_NOTE,"  DATA_SIZE(*dlMpp) = %d", DATA_SIZE(*dlMpp));
		{	int i;
			for(i=0; i<dlInfo->ADDR_length; i++) {
				/*
				 *+ Ditto
				 */
				cmn_err(CE_CONT,"%X ", dlInfo->dlAddr[i]);
			}
			/*
			 *+ Ditto
			 */
			cmn_err(CE_NOTE,"");
			for(i=0; i<sizeof(dlInfo_t); i++) {
				/*
				 *+ Ditto
				 */
				cmn_err(CE_CONT,"%X ", (uint8)*((uint8 *)dlInfo + i));
			}
			/*
			 *+ Ditto
			 */
			cmn_err(CE_NOTE,"");
			for(i=0; i<DATA_SIZE(*dlMpp); i++) {
				/*
				 *+ Ditto
				 */
				cmn_err(CE_CONT,"%X ",
						(uint8)*((uint8 *)(*dlMpp)->b_rptr + i));
			}
			/*
			 *+ Ditto
			 */
			cmn_err(CE_NOTE,"");
			/*
			 *+ Ditto
			 */
			cmn_err(CE_NOTE,"");
			for(i=sizeof(dlInfo_t);i <DATA_SIZE(*dlMpp); i++) {
				/*
				 *+ Ditto
				 */
				cmn_err(CE_CONT,"%X ", (uint8)*((uint8 *)dlInfo + i));
			}
			/*
			 *+ Ditto
			 */
			cmn_err(CE_NOTE,"");
		}
#endif /* DLPI_DEBUG */
	} else {
		*dlMpp = NULL;
		cCode = 1;
	}
	return(NTR_LEAVE(cCode));
}

/*
 * int LipmxCopyLanNode(uint32 lan, uint8 *nodep)
 *	Called as a result of an M_IOCTL/IPX_GET_NODE_ADDR
 *
 * Calling/Exit State:
 *	
 *	
 */
int
LipmxCopyLanNode(uint32 lan, uint8 *nodep)
{
	pl_t	lvl;

	NTR_ENTER(2, lan, nodep, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxCopyLanNode: lan %d", lan));
	if (lan < ConfiguredLans) {
		lvl = LOCK(LipmxLans[lan].lanLock, plstr);
		IPXCOPYNODE(LipmxLans[lan].ipxNode, nodep);
		UNLOCK(LipmxLans[lan].lanLock, lvl);
		return(NTR_LEAVE(0));
	}
	return(NTR_LEAVE(1));
}

/* Possibly Called as a result of an M_IOCTL/IPX_GET_NET
 */
/*
 * int LipmxCopyLanNet(uint32 lan, uint32 *netp)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
int
LipmxCopyLanNet(uint32 lan, uint32 *netp)
{
	pl_t	lvl;

	NTR_ENTER(2, lan, netp, 0, 0, 0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"Enter LipmxCopyLanNet: lan %d", lan));
	if (lan < ConfiguredLans) {
		lvl = LOCK(LipmxLans[lan].lanLock, plstr);
		IPXCOPYNET(&LipmxLans[lan].ipxNet, netp);
		UNLOCK(LipmxLans[lan].lanLock, lvl);
		return(NTR_LEAVE(0));
	}
	return(NTR_LEAVE(1));
}

/*
 * int lipmxlwsrv(queue_t *q)
 *	Handles downstream flow-control and packet-pacing
 *	for replaceable-router responses (bcast/directed).
 *
 * Calling/Exit State:
 *	
 *	
 */
int
lipmxlwsrv(queue_t *q)
{
	mblk_t	*mp;
	Lan_t	*lanInfo = (Lan_t *)(q->q_ptr);
	uint32	lbolt;
	pl_t	lvl;

	NTR_ENTER(1, q, 0, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
				"Enter lipmxlwsrv"));
	if (lanInfo == NULL) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"lipmxlwsrv: q->q_ptr is NULL"));
		return((int)NTR_LEAVE(0));
	}

	lanInfo->sendId = (toid_t)0;

	while (mp = getq(q)) {
		switch(MTYPE(mp)) {
		default:
			/*
			 *+ Unknown packet type received from upstream
			 */
			cmn_err(CE_WARN, "lipmxlwsrv: Bad packet type 0x%x",
					MTYPE(mp));
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"Freeing bad packet type 0x%x", MTYPE(mp)));
			freemsg(mp);
			break;
		case M_PROTO:
			lvl = LOCK(lanInfo->lanLock, plstr);
			if (lanInfo->qbot) {
				if (canputnext(q)) {
					NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
						"lipmxlwsrv: pkt to wire"));
					ATOMIC_INT_INCR(&lanInfo->qcount);
					UNLOCK(lanInfo->lanLock, lvl);
					putnext(q, mp);
					ATOMIC_INT_DECR(&lanInfo->qcount);
					continue;
				} else {
					UNLOCK(lanInfo->lanLock, lvl);
					NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
							"lipmxlwsrv: flow-control - canput fail"));
					putbq(q, mp);
					goto endWhile;
				}
			} else {
				NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
						"lipmxlwsrv: bottom que is NULL"));
				UNLOCK(lanInfo->lanLock, lvl);
				freemsg(mp);
				continue;		/* get the rest of the msg off que */
			}
			/*NOTREACHED*/
			break;
		}
	}
endWhile:
	/* 
	** Send Paced packets if its time for them
	*/
	drv_getparm(LBOLT, &lbolt);
	if (((lbolt - lanInfo->lastPacedSend) >= lanInfo->paceTicks) && 
			(lanInfo->bcastPaced || lanInfo->directedPaced)) {

		NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
			"lipmxlwsrv: time for next paced packet"));

		lvl = LOCK(lanInfo->lanLock, plstr);
		if (lanInfo->directedPaced) {
			NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
				"lipmxlwsrv: Get next directed Paced packet"));
			mp = LIPMXTakeItOff( &lanInfo->directedPaced);
#ifdef DEBUG
			if (!mp) {
				UNLOCK(lanInfo->lanLock, lvl);
				cmn_err(CE_WARN,"lipmxlwsrv: LIPMXTakeItOff returned NULL mp");
				lanInfo->sendId = (toid_t)0;
				return((int)NTR_LEAVE(0));
			}
#endif
			if (lanInfo->qbot) {
				if (canputnext(q)) {
					NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
						"lipmxlwsrv: Send directedPaced packet mp 0x%x",mp));
					ATOMIC_INT_INCR(&lanInfo->qcount);
					UNLOCK(lanInfo->lanLock, lvl);
					putnext(q, mp);
					ATOMIC_INT_DECR(&lanInfo->qcount);
					lvl = LOCK(lanInfo->lanLock, plstr);
				} else {
					NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
						"lipmxlwsrv: canput! directedPaced packet mp 0x%x",
						mp));
					putq(q, mp);
				}
			} else {
				NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
					"lipmxlwsrv: directedPaced mp, qbot is NULL"));
				freemsg(mp);
			}
		}
		if (lanInfo->bcastPaced) {
			NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
				"lipmxlwsrv: Get next bcast Paced packet"));
			mp = LIPMXTakeItOff( &lanInfo->bcastPaced);
#ifdef DEBUG
			if (!mp) {
				UNLOCK(lanInfo->lanLock, lvl);
				cmn_err(CE_WARN,"lipmxlwsrv: LIPMXTakeItOff returned NULL mp");
				lanInfo->sendId = (toid_t)0;
				return((int)NTR_LEAVE(0));
			}
#endif
			if (lanInfo->qbot) {
				if (canputnext(q)) {
					NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
						"lipmxlwsrv: Send bcastPaced packet mp 0x%x",mp));
					ATOMIC_INT_INCR(&lanInfo->qcount);
					UNLOCK(lanInfo->lanLock, lvl);
					putnext(q, mp);
					ATOMIC_INT_DECR(&lanInfo->qcount);
				} else {
					NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
						"lipmxlwsrv: canput! bcastPaced packet mp 0x%x",
						mp));
					UNLOCK(lanInfo->lanLock, lvl);
					putq(q, mp);
				}
			} else {
				NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
					"lipmxlwsrv: bcastPaced mp, qbot is NULL"));
				UNLOCK(lanInfo->lanLock, lvl);
				freemsg(mp);
			}
		} else {
			UNLOCK(lanInfo->lanLock, lvl);
		}
		lanInfo->lastPacedSend = lbolt;
	}
	/* 
	** If we still have PACED packets  set timer for packet delay ticks.
	** If no PACED packets left, don't restart timer. When more packets 
	** are put on private list lipmxlwsrv() routine will be qenabled.
	*/
	if ((lanInfo->bcastPaced) || (lanInfo->directedPaced)) {
		lanInfo->sendId = itimeout(qenable, q, lanInfo->paceTicks, pltimeout);
	} 

	return((int)NTR_LEAVE(0));
}
/*
 * mblk_t * LIPMXTakeItOff( mblk_t **topPtr)
 *	Lan_t lock must be held on entry. 
 *	Does NOT release the lock.
 *
 * Calling/Exit State:
 *	
 *	
 */
mblk_t *
LIPMXTakeItOff( mblk_t **topPtr)
{	
	mblk_t	*topMp, *mp = NULL;

	NTR_ENTER(1, topPtr, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LIPMXTakeItOff"));
	topMp = *topPtr;
#ifdef DEBUG
	if (topMp == NULL) {
		/*
		 *+ DEBUG, display internal code error
		 */
		cmn_err(CE_WARN, "LIPMXTakeItOff: NO mp on list");
		return((mblk_t *)NTR_LEAVE(mp));
	}
#endif
	/* 
	** If only one on the list take it off and NULL topPtr
	*/
	mp = topMp;
	if (topMp == topMp->b_next) {
		NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
				"LIPMXTakeItOff: taking off Last mp",mp));
		*topPtr = NULL;
	} else {
		topMp->b_prev->b_next = topMp->b_next;
		topMp->b_next->b_prev = topMp->b_prev;
		*topPtr = topMp->b_next;
	}
	NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
			"LIPMXTakeItOff: returning mp 0x%X",mp));
	mp->b_next = NULL;
	mp->b_prev = NULL;
	return((mblk_t *)NTR_LEAVE(mp));
}

/*
 * void DivestRRouterLanKeys(void)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
void
DivestRRouterLanKeys(void)
{	uint32	lan;

	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
				"Enter DivestRRouterLanKeys"));

	for(lan = 0; lan < ConfiguredLans; lan++) {
		ATOMIC_INT_INCR(&RRIPX.Ripcount);
		LipmxLans[lan].rrLanKey
			= RROUTER.InvalidateLanKey(LipmxLans[lan].rrLanKey);
		ATOMIC_INT_DECR(&RRIPX.Ripcount);
	}
	NTR_VLEAVE();
	return;
}

/*
 * void RegisterLansWithRRouter(void)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
void
RegisterLansWithRRouter(void)
{	uint32	lan;
	pl_t	lvl;

	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
				"Enter RegisterLansWithRRouter"));

	for(lan = 0; lan < ConfiguredLans; lan++) {
		lvl = LOCK(LipmxLans[lan].lanLock, plstr);
		LipmxBuildRRlanKey(lan, lvl);
	}

	NTR_VLEAVE();
	return;
}

/*
 *	Replaceable Router Methods Follow
 */
/*
 * void LIPMXsendData(mblk_t *mp, void **ipxLanKeyPtr, uint8 flags)
 *	Generalized function to send packet to correct destination
 *
 * Calling/Exit State:
 *	
 *	
 */
void
LIPMXsendData(mblk_t *mp, void **ipxLanKeyPtr, uint8 flags)
{
	Lan_t	*lanInfo;
	uint32	outLan;
	register ipxHdr_t 	*ipxHeader;
	register mblk_t	*dlHdr;
	register mblk_t	*topMp;
	mblk_t	*dupMp;
	queue_t	*q;
	mblk_t	**topPtr;
	uint16	len;
	pl_t	lvl;

	NTR_ENTER(3, mp, ipxLanKeyPtr, flags, 0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LIPMXsendData: ipxLanKeyPtr 0x%X, flags 0x%X",
			ipxLanKeyPtr, flags));

	lipmxStats.OutTotal++;
	/* Pkt size has been guaranteed to be at least
	** sizeof(IpxHdr_t) by callers.
	*/
#ifdef DEBUG
	if (mp == NULL) {
		/*
		 *+ DEBUG, display internal code error
		 */
		cmn_err(CE_WARN,"LIPMXsendData: NULL mp !?");
		NTR_VLEAVE();
		return;
	}
#endif
	/*
	** Check to see if this packet goes to one of my nets or if it
	** needs to be routed to a remote net.
	*/
	ipxHeader = (ipxHdr_t *)mp->b_rptr;
	if (ipxLanKeyPtr != NULL) {
		outLan = (uint32)*ipxLanKeyPtr;
	} else {
#ifdef DEBUG
		/*
	 	** If the destination socket is zero a BIG mistake has
	 	** been made somewhere.  Drop the packet and scream.
	 	*/
		if ((ipxHeader->dest.sock[0] == 0)
				&& (ipxHeader->dest.sock[1] == 0)) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
					"HUH?LIPMXsendData: Drop pkt whose destSock == 0"));
			/*
			 *+ Application didn't fill in socket
			 */
			cmn_err(CE_WARN,
				"LIPMXsendData: Destination Socket is zero !?");
			freemsg(mp);
			NTR_VLEAVE();
			return;
		}
#endif /* DEBUG */

		/*
	 	** If the destination net is zero the packet initiator must not
	 	** know what network he is on.  This is probably a broadcast of
	 	** some type to find out what services are available.
	 	*/
		if (PGETINT32(ipxHeader->dest.net) == 0) {
			NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
					"LIPMXsendData: Destination Net of 0 fill in dest.net"));

			IPXCOPYNET( &LipmxLans[0].ipxNet, ipxHeader->dest.net);
			lipmxStats.OutFillInDest++;

			if (lipmxStats.IpxInternalNet == 1) {
				NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
					"LIPMXsendData: Dest.net 0, Internal net, fill in node"));
				/*
				** We have a internal net, send this packet upstream
				*/
				IPXCOPYNODE( &LipmxLans[0].ipxNode, ipxHeader->dest.node);
			} else {
				NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
					"LIPMXsendData: Dest.net 0, No internal net"));
				/*
				**  No internal net, Node zero, 
				**  fill in node with broadcast and route to socket
				*/
				if( IPXCMPNODE( ZEROHOST, ipxHeader->dest.node)) {
					NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
						"LIPMXsendData: Fill in dest.node with Broadcast"));
					IPXCOPYNODE( ALLHOSTS, ipxHeader->dest.node);
				}
			}
		}
	
		/*
		**	If this packet is to the internal net, send to lan[0]
		*/
		if (IPXCMPNET(&LipmxLans[0].ipxNet, ipxHeader->dest.net) &&
				IPXCMPNODE(LipmxLans[0].ipxNode, ipxHeader->dest.node)) {
			outLan = 0;
		} else {
			/*
			** Get routing info for the destination net.
			** If it is NULL drop the packet.
			*/
			ATOMIC_INT_INCR(&RRIPX.Ripcount);
			dlHdr = RROUTER.GetRouteInfo(ipxHeader, (void **)&outLan);
			ATOMIC_INT_DECR(&RRIPX.Ripcount);
			if (dlHdr != NULL) {
				dlHdr->b_cont = mp;
				mp = dlHdr;
			}
		}
	}

	if (outLan == 0) {
		if( IPXCMPSOCK( ipxHeader->dest.sock, ipxHeader->src.sock)) {
			/*
			**	Drop all packets from the internal net to the internal net,
			**	and with the same src/dest socket.  Note: during
			**	initialization a one RIP packet for each net will
			**	come thru here and get dropped.  That is expected.
			*/
			NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
				"LIPMXsendData: sending to internal net, dest/src sock 0x%X, dropping",
				PGETINT16( ipxHeader->dest.sock)));
			lipmxStats.OutSameSocket++;
			freemsg( mp);
			NTR_VLEAVE();
			return;
		}
		NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
			"LIPMXsendData: sending to internal net"));
		IpxRouteDataToSocket(mp);
		lipmxStats.OutInternal++;
		NTR_VLEAVE();
		return;
	}
	/* Insure router gave us a good lan index */
	if (outLan >= ConfiguredLans) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LIPMXsendData: RROUTER reports bad lan[0x%X] for net 0x%X",
			outLan, PGETINT32(ipxHeader->dest.net)));
		lipmxStats.OutBadLan++;
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	/*
	 * send a copy of boadcast packets to the socket router 
	 * if no internal net and the src/dest sockets are different.
	 */
	if ( (IPXCMPNODE( ALLHOSTS, ipxHeader->dest.node))&& 
			(IPXCMPNET(&LipmxLans[0].ipxNet, ipxHeader->dest.net)) &&
			(! IPXCMPSOCK( ipxHeader->dest.sock, ipxHeader->src.sock)) ) {
		/* dont dup dlHdr */
		if( (dupMp = dupmsg( mp->b_cont)) != NULL) {
			NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
				"LIPMXsendData: Bcast to local lan with no internal net, send dupmsg upstream to socket 0x%X", PGETINT16(ipxHeader->dest.sock)));
			IpxRouteDataToSocket(dupMp);
			lipmxStats.OutInternal++;
		}
	}

	/* outLan != 0, qbot is the WR() q
	*/
	lvl = LOCK(LipmxLans[outLan].lanLock, plstr);
	ipxHeader = (ipxHdr_t *)mp->b_cont->b_rptr;
	len = (uint16)msgdsize(mp->b_cont);
	if ((len > LipmxLans[outLan].dlInfo.SDU_max) || 
			(len < (uint16)PGETINT16(&ipxHeader->len)) ) {
		UNLOCK(LipmxLans[outLan].lanLock, lvl);
#ifdef DEBUG
		/*
		 *+ Application sent message that is too long
		 */
		cmn_err(CE_WARN,"LIPMXsendData: Bad message length %d ipxHdr->len =%d",
				len, (uint16)PGETINT16(&ipxHeader->len));
#endif
		NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
			"LIPMXsendData: Bad message length %d ipxHdr->len =%d",
				len, (uint16)PGETINT16(&ipxHeader->len)));
		lipmxStats.OutBadSize++;
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	q = LipmxLans[outLan].qbot;
	if (q == NULL) {
		UNLOCK(LipmxLans[outLan].lanLock, lvl);
		NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
			"LIPMXsendData: qbot in NULL, outLan 0x%X", outLan));
		lipmxStats.OutNoLan++;
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}
	lanInfo = (Lan_t *)(q->q_ptr);
	if (lanInfo == NULL) {
		UNLOCK(LipmxLans[outLan].lanLock, lvl);
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LIPMXsendData: q->q_ptr is NULL"));
		lipmxStats.OutNoLan++;
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	if (flags & RR_PACE) {
		lipmxStats.OutPaced++;
		if (flags & RR_BCAST) {
			NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
					"LIPMXsendData: queueing paced bcast"));
			topPtr = &LipmxLans[outLan].bcastPaced;
			topMp = LipmxLans[outLan].bcastPaced;
		} else {
			NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
					"LIPMXsendData: queueing paced directed"));
			topPtr = &LipmxLans[outLan].directedPaced;
			topMp = LipmxLans[outLan].directedPaced;
		}

		/*
		** put Paced packet on link list
		*/
		if (topMp) {
			topMp->b_prev->b_next = mp;
			mp->b_prev = topMp->b_prev;
			topMp->b_prev = mp;
			mp->b_next = topMp;
		} else {
			mp->b_next = mp;		/* this is the first one on the list */
			mp->b_prev = mp;		/* point it to itself */
			*topPtr = mp;
		}
		if (lanInfo->sendId == 0)
			qenable(q);
		UNLOCK(LipmxLans[outLan].lanLock, lvl);
	} else {
		if( flags & RR_QUEUE) {
			NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
				"LIPMXsendData: Queue packet: RR_QUEUE - queue to srv()"));
			UNLOCK(LipmxLans[outLan].lanLock, lvl);
			lipmxStats.OutQueued++;
			putq(q, mp);
		} else {
			if (canputnext(q)) {
				NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
						"LIPMXsendData: putting non-paced on wire"));
				lipmxStats.OutSent++;
				ATOMIC_INT_INCR(&LipmxLans[outLan].qcount);
				UNLOCK(LipmxLans[outLan].lanLock, lvl);
				putnext(q, mp);
				ATOMIC_INT_DECR(&LipmxLans[outLan].qcount);
			} else {
				NWSLOG((LIPMXID,0,PNW_DROP_PACKET,SL_TRACE,
					"LIPMXsendData: Queue packet: can't send down just now - queue to srv()"));
				UNLOCK(LipmxLans[outLan].lanLock, lvl);
				lipmxStats.OutQueued++;
				putbq(q, mp);
			}
		}
	}
	NTR_VLEAVE();
	return;
}

/*
 * uint32 LIPMXmapSapLanToNetwork(uint32 sapConnectedLan)
 *	description here
 *
 * Calling/Exit State:
 *	
 *	
 */
uint32
LIPMXmapSapLanToNetwork(uint32 sapConnectedLan)
{
	NTR_ENTER(1, sapConnectedLan, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LIPMXmapSapLanToNetwork"));

	if(sapConnectedLan <= ConfiguredLans)
		if(LipmxLans[sapConnectedLan].ipxNet != (uint32)-1)
			return(NTR_LEAVE(LipmxLans[sapConnectedLan].ipxNet));
	return(NTR_LEAVE(0));
}

/*
 * void * LIPMXuseMethodWithLanKey(void *ipxLanKey, void *rrPrivateKey,
 *	If ipxLanKey == ALL_LANS, cycle through lans/lanKeys calling method
 *
 * Calling/Exit State:
 *	
 *	
 */
void *
LIPMXuseMethodWithLanKey(void *ipxLanKey, void *rrPrivateKey,
						void *rrouterMethod(void *rrPrivateKey,
											void *rrLanKey))
{	uint32	i, lan = (uint32)ipxLanKey;
	void	*rtn = NULL;

	NTR_ENTER(3, ipxLanKey, rrPrivateKey, rrouterMethod,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
				"LIPMXuseMethodWithLanKey: ipxLanKey 0x%X",
				(uint32)ipxLanKey));
	if (lan == (uint32)ALL_LANS) {
		for(i = 0; i < ConfiguredLans; i++)
			if (LipmxLans[i].rrLanKey) {
				ATOMIC_INT_INCR(&RRIPX.Ripcount);
				(void *)rrouterMethod(rrPrivateKey, LipmxLans[i].rrLanKey);
				ATOMIC_INT_DECR(&RRIPX.Ripcount);
			} else {
				NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
							"ipxLanKey %d has no rrLanKey !?", i));
			}
#ifdef RR_DEBUG
	else
		/*
		 *+ DEBUG - no lan key
		 */
		cmn_err(CE_NOTE,
			"LIPMXuseMethodWithLanKey: Lan[%d] has no rrLanKey", i);
#endif /* RR_DEBUG */
	} else {
		if ((lan < ConfiguredLans) && LipmxLans[lan].rrLanKey) {
			ATOMIC_INT_INCR(&RRIPX.Ripcount);
			rtn = (void *)rrouterMethod(rrPrivateKey, 
									LipmxLans[lan].rrLanKey);
			ATOMIC_INT_DECR(&RRIPX.Ripcount);
		}
	}
	return((void *)NTR_LEAVE(rtn));
}

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

#ident	"@(#)kern:net/nw/ipx/lipmx/lipmx_streams.c	1.5"
#ident	"$Id: lipmx_streams.c,v 1.3.2.1 1994/11/08 20:49:32 vtag Exp $"

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
#include <net/nw/ipx/lipmx/lipmx_ioctls.h>
#include <net/nw/ipx/lipmx/norouter.h>
#else
#include "lipmx.h"
#include "lipmx_ioctls.h"
#include "norouter.h"
#endif /* _KERNEL_HEADERS */

/*
 * int lipmxuwput(queue_t *q, mblk_t *mp)
 *	The Upper Write Put procedure is responsible for handling all
 *	IOCTL/MCTL calls coming downstream.  If an IOCTL is not recognized
 *	it is dropped.
 *
 *	The datagram message should come downstream with only an M_DATA 
 *	message (no control portion) and will include the 
 *	IPX HEADER information.
 *	The upper put procedure will scan the router tables to determine
 *	which lower connected stream should receive each datagram.  The
 *	IPX network number is used to associate a given stream with a
 *	connected lan.  If the destination network is not a connected
 *	lan, it will be routed through the connected lan that received
 *	the advertisment with the fewest ticks and or hops to that network.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
int
lipmxuwput(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"lipmxuwput: ENTER"));

	switch(MTYPE(mp)) {
	/* Valid when a bound IPX socket has outgoing
	** data, or sap has opened ipxr directly
	*/
	/* SetRipQ ??? */
	/* if !UnderIpx, should we drop non-rip/sap type 20 ??? */
	/* ipxuwput filters these ??? */
	case M_DATA:
		NWSLOG((LIPMXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"lipmxuwput: M_DATA size %d",DATA_SIZE(mp)));
		lipmxStats.OutTotalStream++;
		if( LipmxTrimPacket(mp) != NULL) {
			if (((ipxHdr_t *)mp->b_rptr)->pt == IPX_PROPAGATION_TYPE) {
				LipmxDupBroadcastPacket(NULL, mp);
			}
			LIPMXsendData(mp, NULL, 0);
		}
		break;
	case M_IOCTL:
		NWSLOG((LIPMXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"lipmxuwput: M_IOCTL size %d ",DATA_SIZE(mp)));
		lipmxStats.Ioctl++;
		switch (((struct iocblk *)mp->b_rptr)->ioc_cmd) {
		case IPX_SET_CONFIGURED_LANS:
			lipmxStats.IoctlSetLans++;
			LipmxIocSetConfiguredLans(q,mp);
			break;
		case IPX_GET_CONFIGURED_LANS :
			lipmxStats.IoctlGetLans++;
			LipmxIocGetConfiguredLans(q, mp);
			break;
		case IPX_SET_SAPQ:
			lipmxStats.IoctlSetSapQ++;
			LipmxIocSetSapQ(q,mp);
			break;
		case IPX_SET_LAN_INFO:
			lipmxStats.IoctlSetLanInfo++;
			LipmxIocSetLanInfo(q,mp);
			break;
		case IPX_GET_LAN_INFO:
			lipmxStats.IoctlGetLanInfo++;
			LipmxIocGetLanInfo(q,mp);
			break;
		case IPX_GET_NET:
			lipmxStats.IoctlGetNetAddr++;
			LipmxIocGetNetAddr(q,mp);
			break;
		case IPX_GET_NODE_ADDR:
			lipmxStats.IoctlGetNodeAddr++;
			LipmxIocGetNodeAddr(q,mp);
			break;
		case IPX_STATS:
			lipmxStats.IoctlGetStats++;
			LipmxIocStats(q,mp);
			break;
		case I_LINK:
			lipmxStats.IoctlLink++;
			LipmxIocLink(q,mp);
			break;
		case I_UNLINK:
			lipmxStats.IoctlUnlink++;
			LipmxIocUnlink(q,mp);
			break;
		default:
			lipmxStats.IoctlUnknown++;
			LipmxBogusIoctl(q,mp);
			break;
		}
		break;
	case M_CTL:
		NWSLOG((LIPMXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"lipmxuwput: M_CTL size %d ",DATA_SIZE(mp)));
		switch (((struct iocblk *)mp->b_rptr)->ioc_cmd) {
		case GET_IPX_INT_NET_NODE:
			LipmxMctlGetIntNetNode(q,mp);
			break;
		case SPX_GET_IPX_MAX_SDU:
			LipmxMctlGetMaxSDU(q, mp);
			break;
		default:
			lipmxStats.IoctlUnknown++;
			LipmxBogusMctl(q,mp);
			break;
		}
		break;

	case M_FLUSH:
	   NWSLOG((LIPMXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"lipmxuwput: M_FLUSH size %d",DATA_SIZE(mp)));
		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		if (*mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
			qreply(q, mp);
		} else {
			freemsg(mp);
		}
		break;

	default:
		NWSLOG((LIPMXID,0,PNW_DROP_PACKET,SL_TRACE,
			"lipmxuwput: Drop pkt, unknown MTYPE 0x%X", MTYPE(mp)));
		freemsg(mp);
		break;

	}
	NWSLOG((LIPMXID,0,PNW_EXIT_ROUTINE,SL_TRACE,
		"lipmuxuwput exit"));

	return((int)NTR_LEAVE(0));
}

/*
 * int lipmxlrput(queue_t *q, mblk_t *mp)
 *	The lower read put procedure will handle all messages coming upstream
 *	from the DLPI Lan driver.  If the message is not reconized it will
 *	be dropped.  Lipmx will determine what to do with the message by calling
 *	the ROUTER.  The message will be sent up to IPX, out another lan or 
 *	it could be dropped if there is no known route to the packets destination
 *	address.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
int
lipmxlrput(queue_t *q, mblk_t *mp)
{	uint32 				DL_type;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"lipmxlrput: ENTER q %xh mp %xh",q,mp));

	switch(MTYPE(mp)) {
	case M_PROTO:
		NWSLOG((LIPMXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"lipmxlrput:  M_PROTO size %d",DATA_SIZE(mp)));

		if (DATA_SIZE(mp) < sizeof(DL_type)) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"lipmxlrput: Drop M_PROTO pkt, no dlpitype, size 0x%X",
				DATA_SIZE(mp)));
			lipmxStats.InProtoSize++;
			freemsg(mp);
			break;
		}

		DL_type = *(uint32 *)(mp->b_rptr);

		switch(DL_type) {
		case DL_UNITDATA_IND:
			LipmxDlUnitdataInd(q,mp);
			break;
		default: /* DL_type */
			NWSLOG((LIPMXID,0,PNW_SWITCH_DEFAULT,SL_TRACE,
				"lipmxlrput: Drop M_PROTO pkt, unknown dlpi type 0x%X",
				DL_type));
			freemsg(mp);
			lipmxStats.InBadDLPItype++;
			break;
		}
		break;

	case M_FLUSH:
		NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,"lipmxlrput: M_FLUSH"));
		/*
		** Flush queues. NOTE: sense of tests is reversed
		** since we are acting like a "stream head".
		*/
		if (*mp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);
		if (*mp->b_rptr & FLUSHW) {
			flushq(WR(q), FLUSHDATA);
			*mp->b_rptr &= ~FLUSHR;
			qreply(q, mp);
		} else {
			freemsg(mp);
		}
		break;

	/* error from below set lankey INVALID */
	case M_ERROR:
	case M_HANGUP:
		NWSLOG((LIPMXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"lipmxlrput: M_ERROR/M_HANGUP size %d",DATA_SIZE(mp)));
		/* invalid lankey  */
		LipmxStreamError(q,mp);
		break;

	case M_PCPROTO:
#ifdef DEBUG
		NWSLOG((LIPMXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"lipmxlrput: M_PCPROTO size %d",DATA_SIZE(mp)));

		if (DATA_SIZE(mp) < sizeof(DL_type)) {
			NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"lipmxlrput: Drop M_PCPROTO pkt, no dlpitype 0x%X",
				DATA_SIZE(mp)));
			freemsg(mp);
			break;
		}

		DL_type = *(uint32 *)(mp->b_rptr);
		switch(DL_type) {
		case DL_OK_ACK:
			NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
					"lipmxlrput: DL_OK_ACK: freeing"));
			break;
		case DL_ERROR_ACK:
			NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
					"lipmxlrput: DL_ERROR_ACK: freeing"));
			{	dl_error_ack_t	*error = (dl_error_ack_t *)(mp->b_rptr);
				cmn_err(CE_WARN,
					"lipmxlrput: prim in error=%d, dl_error=%d, unix_err=%d",
					error->dl_error_primitive, error->dl_errno,
					error->dl_unix_errno);
			}
			break;
		default:
			NWSLOG((LIPMXID,0,PNW_SWITCH_DEFAULT,SL_TRACE,
				"lipmxlrput: M_PCPROTO unknown DL_type: %xh",DL_type));
			break;
		}	
		freemsg(mp);
		break;
#endif /* def DEBUG */

	case M_DATA:
	default: /* message type */
		NWSLOG((LIPMXID,0,PNW_SWITCH_DEFAULT,SL_TRACE,
			"lipmxlrput: unknown MTYPE %d - dropping", MTYPE(mp)));
		freemsg(mp);
		break;
	}

	NWSLOG((LIPMXID,0,PNW_EXIT_ROUTINE,SL_TRACE,
		"lipmxlrput: EXIT "));

	return((int)NTR_LEAVE(0));
}

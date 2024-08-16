/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ripx/ripx_ioctls.c	1.8"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: ripx_ioctls.c,v 1.10 1994/05/05 15:53:35 vtag Exp $"

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
 * ripx_ioctls.c	
 *
 * The calls in this module just "pre-screen" the IOCTL prior
 * to passing control to a appropriate handler in rip.c. 
 *
 * Lock usage - Code/structures protected:	NONE
 *  ALL FUNCTIONS: No locks held on entry or exit. No locks are
 *  used, as this module has no code/data that needs to be
 *  protected.
 */
#ifdef _KERNEL_HEADERS
#include <net/nw/ripx/rip.h>	/* RIPXID, SL_TRACE, ... */
#include <net/nw/ripx/ripx_ioctls.h>
#include <net/nw/ripx/ripx_streams.h>
#include <net/nw/ripx/rrrip.h>	 /* RIPXID, SL_TRACE, ... */
#else
#include "rip.h"    /* RIPXID, SL_TRACE, ... */
#include "ripx_ioctls.h"
#include "ripx_streams.h"
#include "rrrip.h"   /* RIPXID, SL_TRACE, ... */
#endif /* _KERNEL_HEADERS */

extern uint8	routerType;
extern RtInfoTable_t	DefaultRIT;
extern RtInfoTable_t	RIT;


/*
 * void RIPStartRouter(queue_t *q, mblk_t *mp)
 *	Start RIPing.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RIPStartRouter(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	/* ioctl root only usage */
	if (!IS_PRIV(q)) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPStartRouter: non root access attempted"));
		RipIocNegAck(q,mp,EPERM);
		NTR_VLEAVE();
		return;
	}
	RIPstart(q);
	RipIocAck(q,mp,0);

	NTR_VLEAVE();
	return;
}

/*
 * void RIPSetRouterType(queue_t *q, mblk_t *mp)
 *	Set Router type FULL or CLIENT
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RIPSetRouterType(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	/* ioctl root only usage */
	if (!IS_PRIV(q)) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPSetRouterType: non root access attempted"));
		RipIocNegAck(q,mp,EPERM);
		NTR_VLEAVE();
		return;
	}

	if (mp->b_cont == NULL) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPSetRouterType: NULL b_cont"));
		RipIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	routerType = *mp->b_cont->b_rptr; 
	NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
            "RIPSetRouterType: routerType set to %d", routerType));
	RipIocAck(q,mp,sizeof(uint8));
	NTR_VLEAVE();
	return;
}
/*
 * void RIPInitRouter(queue_t *q, mblk_t *mp)
 *	Setup RIP parameters.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RIPInitRouter(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	/* ioctl root only usage */
	if (!IS_PRIV(q)) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPInitRouter: non root access attempted"));
		RipIocNegAck(q,mp,EPERM);
		NTR_VLEAVE();
		return;
	}

	if ((mp->b_cont == NULL)
			|| (DATA_SIZE(mp->b_cont) < sizeof(Initialize_t))) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPInitRouter: NULL/mis-sized data area for %d bytes",
			sizeof(Initialize_t)));
		RipIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	if (!RIPInitialize((Initialize_t *)mp->b_cont->b_rptr))
		RipIocNegAck(q,mp,EINVAL);
	else
		RipIocAck(q,mp,sizeof(Initialize_t));
	NTR_VLEAVE();
	return;
}

/*
 * void RIPIResetRouter(queue_t *q, mblk_t *mp)
 *	Initiate a Router reset.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RIPIResetRouter(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	/* ioctl root only usage */
	if (!IS_PRIV(q)) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPIResetRouter: non root access attempted"));
		RipIocNegAck(q,mp,EPERM);
		NTR_VLEAVE();
		return;
	}

	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	if (&RIT == &DefaultRIT) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPIResetRouter: RIT is defaultRIT, NAK"));
		ATOMIC_INT_DECR(&RRIPX.Ripcount);
		RipIocNegAck(q,mp,EAGAIN);
		NTR_VLEAVE();
		return;
	}
	RipResetRouter();
	ATOMIC_INT_DECR(&RRIPX.Ripcount);
	RipIocAck(q,mp,0);
	NTR_VLEAVE();
	return;
}

/*
 * void RIPIDownRouter(queue_t *q, mblk_t *mp)
 *	Down the router by Deregestering it from the lan multiplexer.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RIPIDownRouter(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	/* ioctl root only usage */
	if (!IS_PRIV(q)) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPIDownRouter: non root access attempted"));
		RipIocNegAck(q,mp,EPERM);
		NTR_VLEAVE();
		return;
	}

	RipDeregisterRouter();

	RipIocAck(q,mp,0);
	NTR_VLEAVE();
	return;
}

/*
 * void RIPIGetNetInfo(queue_t *q, mblk_t *mp)
 *	Return information about a given network from RIP's
 *	Routing Information Table (RIT).
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RIPIGetNetInfo(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	if ((mp->b_cont == NULL)
			|| (DATA_SIZE(mp->b_cont) < sizeof(netInfo_t))) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPIGetNetInfo: not enough data area (%d)",
			mp->b_cont ? DATA_SIZE(mp->b_cont) : 0));
		RipIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	if (&RIT == &DefaultRIT) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPIGetNetInfo: RIT is defaultRIT, NAK"));
		ATOMIC_INT_DECR(&RRIPX.Ripcount);
		RipIocNegAck(q,mp,EAGAIN);
		NTR_VLEAVE();
		return;
	}
	if (!RIPgetNetInfo((netInfo_t *)mp->b_cont->b_rptr))
		RipIocNegAck(q,mp,EINVAL);
	else
		RipIocAck(q,mp, sizeof(netInfo_t));
	ATOMIC_INT_DECR(&RRIPX.Ripcount);
	NTR_VLEAVE();
	return;
}

/*
 * void RIPIGetRouterTable(queue_t *q, mblk_t *mp)
 *	Send up the entire Router Information Table (RIT).
 *	If a failure occurs, RIPgetRouterTable returns a
 *	-1 and the ioctl fails.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RIPIGetRouterTable(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	if (&RIT == &DefaultRIT) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPIGetRouterTable: RIT is defaultRIT, NAK"));
		ATOMIC_INT_DECR(&RRIPX.Ripcount);
		RipIocNegAck(q,mp,EAGAIN);
		NTR_VLEAVE();
		return;
	}
	if (!RIPgetRouterTable(q)) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPIGetRouterTable: RIPgetRouterTable failed"));
		RipIocNegAck(q,mp,EAGAIN);
	} else {
		RipIocAck(q,mp,0);
	}
	ATOMIC_INT_DECR(&RRIPX.Ripcount);
	NTR_VLEAVE();
	return;
}

/*
 * void RIPIInfo(queue_t *q, mblk_t *mp)
 *	Report Router Info - copy the driver-global stats structure.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RIPIInfo(queue_t *q, mblk_t *mp)
{	RouterInfo_t *rtnInfo;

	NTR_ENTER(2, q, mp, 0,0,0);

	if ((mp->b_cont == NULL)
			|| (DATA_SIZE(mp->b_cont) != sizeof(RouterInfo_t))) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RIPIInfo: bad size data area (%d)",
			mp->b_cont ? DATA_SIZE(mp->b_cont) : 0));
		RipIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	rtnInfo = (RouterInfo_t *)mp->b_cont->b_rptr;
	mp->b_cont->b_wptr = mp->b_cont->b_rptr + sizeof(RouterInfo_t);

	/* copy the driver-global stats struct */
	*rtnInfo = ripStats;
	RipHashStats(&(rtnInfo->RITstats));
	RipIocAck(q,mp,sizeof(RouterInfo_t));
	NTR_VLEAVE();
	return;
}

/*
 * void RipIGetHashTableStats(queue_t *q, mblk_t *mp)
 *	Return population and performance statitics for
 *	the Routing Information Table (RIT).
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RipIGetHashTableStats(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	if ((mp->b_cont == NULL)
			|| (DATA_SIZE(mp->b_cont) != sizeof(hashStats_t))) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipIGetHashTableStats: bad size data area (%d)",
			mp->b_cont ? DATA_SIZE(mp->b_cont) : 0));
		RipIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	if (&RIT == &DefaultRIT) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipIGetHashTableStats: RIT is defaultRIT, NAK"));
		ATOMIC_INT_DECR(&RRIPX.Ripcount);
		RipIocNegAck(q,mp,EAGAIN);
		NTR_VLEAVE();
		return;
	}
	RipHashStats((hashStats_t *)mp->b_cont->b_rptr);

	ATOMIC_INT_DECR(&RRIPX.Ripcount);
	RipIocAck(q,mp,0);
	NTR_VLEAVE();
	return;
}

/*
 * void RipIGetRouteTableHashSize(queue_t *q, mblk_t *mp)
 *	Return the size (# of hash buckets) in the Routing
 *	Information Table (RIT).  This is a necessary precursor
 *	to proper operation of RipIDumpHashTable.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RipIGetRouteTableHashSize(queue_t *q, mblk_t *mp)
{	hashStats_t		rtnStats;
	HashTableSize_t *hashTable;

	NTR_ENTER(2, q, mp, 0,0,0);

	if ((mp->b_cont == NULL)
			|| (DATA_SIZE(mp->b_cont) < sizeof(HashTableSize_t))) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipGetRouteTableHashSize: wrong size data area (%d)",
			mp->b_cont ? DATA_SIZE(mp->b_cont) : 0));
		RipIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	RipHashStats(&rtnStats);
	hashTable = (HashTableSize_t *)mp->b_cont->b_rptr;
	hashTable->size = rtnStats.keys;
	RipIocAck(q,mp,0);
	NTR_VLEAVE();
	return;
}

/*
 * void RipIDumpHashTable(queue_t *q, mblk_t *mp)
 *	Send up an array of counts of items in each
 *	bucket of the Routing Information Table (RIT).
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RipIDumpHashTable(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	ATOMIC_INT_INCR(&RRIPX.Ripcount);
	if (&RIT == &DefaultRIT) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipIDumpHashTable: RIT is defaultRIT, NAK"));
		ATOMIC_INT_DECR(&RRIPX.Ripcount);
		RipIocNegAck(q,mp,EAGAIN);
		NTR_VLEAVE();
		return;
	}
	if ((mp->b_cont == NULL)
				|| (RipHashBucketCounts((int *)mp->b_cont->b_rptr,
						(uint16)(DATA_SIZE(mp->b_cont) / sizeof(int)))
					!= 0)) {
		NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
			"RipDumpHashTable: bad size data area (%d)",
			mp->b_cont ? DATA_SIZE(mp->b_cont) : 0));
		ATOMIC_INT_DECR(&RRIPX.Ripcount);
		RipIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	ATOMIC_INT_DECR(&RRIPX.Ripcount);
	RipIocAck(q,mp,0);
	NTR_VLEAVE();
	return;
}

/*
 * void RIPICheckSapSource(queue_t *q, mblk_t *mp)
 *	Check a sap source for validity
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RIPICheckSapSource(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);

	if ((mp->b_cont == NULL)
			|| (DATA_SIZE(mp->b_cont) < sizeof(checkSapSource_t))) {
		NWSLOG((RIPXID, 0, PNW_ERROR, SL_TRACE,
				"RIPICheckSapSource: b_cont NULL/offsize for %d bytes",
				sizeof(checkSapSource_t)));
		RipIocNegAck(q,mp,EINVAL);
	} else {
		RIPcheckSapSource((checkSapSource_t *)mp->b_cont->b_rptr);
		RipIocAck(q,mp,sizeof(checkSapSource_t));
	}
	NTR_VLEAVE();
	return;
}

/*
 * void RipIocNegAck(queue_t *q, mblk_t *mp, int err)
 *	NAK an IOCTL
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RipIocNegAck(queue_t *q, mblk_t *mp, int err)
{
	NTR_ENTER(3, q, mp, err, 0,0);
	NWSLOG((RIPXID,0,PNW_ERROR,SL_TRACE,
		"RipIocNegAck: Negative Acknowledgement of M_IOCTL, error code %d",
		err));
	MTYPE(mp) = M_IOCNAK;
	((struct iocblk *)(mp->b_rptr))->ioc_error = err;
	qreply(q, mp);
	NTR_VLEAVE();
	return;
}

/*
 * void RipIocAck(queue_t *q, mblk_t *mp, uint32 count)
 *	ACK an IOCTL
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RipIocAck(queue_t *q, mblk_t *mp, uint32 count)
{
	NTR_ENTER(3, q, mp, count, 0,0);
	NWSLOG((RIPXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"RipIocAck: Positive Acknowledgement of M_IOCTL, data size %d",
			count));
	MTYPE(mp) = M_IOCACK;
	((struct iocblk *)(mp->b_rptr))->ioc_count = count;
	qreply(q, mp);
	NTR_VLEAVE();
	return;
}

/*
 * void RipIBogusIoctl(queue_t *q, mblk_t *mp)
 *	Note the fact that an unknown IOCTL was encountered.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit
 */
void
RipIBogusIoctl(queue_t *q, mblk_t *mp)
{	struct iocblk	*iocp = (struct iocblk *)(mp->b_rptr);

#ifdef DEBUG
	printf("RIPIBogusIoctl : BOGUS IOCTL 0x%X!\n", iocp->ioc_cmd);
#endif
	NWSLOG((RIPXID,0,PNW_SWITCH_DEFAULT,SL_TRACE,
		"RIPIBogusIoctl: SWITCH M_IOCTL: DEFAULT: 0x%X",iocp->ioc_cmd));
	RipIocNegAck(q,mp,EINVAL);
	return;
}

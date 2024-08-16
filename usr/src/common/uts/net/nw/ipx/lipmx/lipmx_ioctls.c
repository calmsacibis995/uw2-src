/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/lipmx/lipmx_ioctls.c	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: lipmx_ioctls.c,v 1.4 1994/02/18 15:21:06 vtag Exp $"

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
#include <net/nw/ipx/lipmx/lipmx_ioctls.h>
#else
#include "lipmx_ioctls.h"
#endif

extern	lock_t			*sapQLock;
extern	queue_t			*sapQ;		/* q for sapping when !underIpx */
extern	atomic_int_t	sapQcount;	/* count of putnext calls active, sapQ * /
/*
 *  Statistics
 */
IpxLanStats_t	lipmxStats = { 0 };


/*
 * FSTATIC void LipmxIocNegAck(queue_t *q, mblk_t *mp, int32 err)
 *	Send a negative acknowledgement upstream.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
FSTATIC void
LipmxIocNegAck(queue_t *q, mblk_t *mp, int32 err)
{	struct iocblk	*iocp = (struct iocblk *)(mp->b_rptr);

	NTR_ENTER(3, q, mp, err, 0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"LipmxIocNegAck: Negative Acknowledgement of M_IOCTL"));

	MTYPE(mp) = M_IOCNAK;
	iocp->ioc_count = 0;
	iocp->ioc_error = err;
	qreply(q, mp);
	NTR_VLEAVE();
	return;
}

/*
 * FSTATIC void LipmxIocAck(queue_t *q, mblk_t *mp, uint32 count)
 *	Send a negative acknowledgement upstream.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
FSTATIC void
LipmxIocAck(queue_t *q, mblk_t *mp, uint32 count)
{	struct iocblk	*iocp = (struct iocblk *)(mp->b_rptr);

	NTR_ENTER(3, q, mp, count, 0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"LipmxIocAck: Positive Acknowledgement of M_IOCTL"));

	MTYPE(mp) = M_IOCACK;
	iocp->ioc_error = 0;
	iocp->ioc_count = count;
	if (mp->b_cont)
		mp->b_cont->b_wptr = mp->b_cont->b_rptr + count;
	qreply(q, mp);
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxBogusMctl(queue_t *q, mblk_t *mp)
 *	Handle unknown M_CTLs.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxBogusMctl(queue_t *q, mblk_t *mp)
{	
#if defined(DEBUG) || defined(NTR_TRACING)
	struct iocblk	*iocp = (struct iocblk *)(mp->b_rptr);
#endif

	NTR_ENTER(2, q, mp, 0,0,0);
    NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
        "LipmxBogusMctl: SWITCH M_MCTL: DEFAULT: %xh",iocp->ioc_cmd));
#ifdef DEBUG
    cmn_err(CE_WARN,"LipmxBogusMctl: BOGUS MCTL 0x%X!", iocp->ioc_cmd);
#endif
    LipmxIocNegAck(q,mp,EINVAL);
    return;
}

/*
 * void LipmxBogusIoctl(queue_t *q, mblk_t *mp)
 *	Handle unknown M_IOCTLs.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxBogusIoctl(queue_t *q, mblk_t *mp)
{	struct iocblk	*iocp = (struct iocblk *)(mp->b_rptr);

	NTR_ENTER(2, q, mp, 0,0,0);
    NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
        "LipmxBogusIoctl: SWITCH M_MCTL: DEFAULT: %xh",iocp->ioc_cmd));
#ifdef DEBUG
    cmn_err(CE_WARN,"LipmxBogusIoctl: BOGUS IOCTL 0x%X!", iocp->ioc_cmd);
#endif
    LipmxIocNegAck(q,mp,EINVAL);
    return;
}

/*
 * void LipmxMctlGetMaxSDU(queue_t *q, mblk_t *mp)
 *	Return Maxium SDU for a local network.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxMctlGetMaxSDU(queue_t *q, mblk_t *mp)
{
	ipxMctl_t			*mctlp;
	spxGetIpxMaxSDU_t	*getSDUp;

	NTR_ENTER(2, q, mp, 0,0,0);
    NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
        "LipmxMctlGetMaxSDU: Enter"));

	if(DATA_SIZE(mp) != sizeof(ipxMctl_t)) {
		NWSLOG((LIPMXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"LipmxMctlGetMaxSDU: M_CTL bad size %d ", DATA_SIZE(mp)));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}
	mctlp = (ipxMctl_t *)mp->b_rptr;
	getSDUp = (spxGetIpxMaxSDU_t *)&(mctlp->mctlblk.u_mctl);
	getSDUp->maxSDU = LipmxGetMaxSDU(getSDUp->network);
	NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
		"LipmxMctlGetMaxSDU: Returning max_SDU %d",
		getSDUp->maxSDU));
	qreply(q, mp);
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxMctlGetIntNetNode(queue_t *q, mblk_t *mp)
 *	Someone linked above needs to know our internal
 *	net/node address. Requester should check net to make sure it's not == 0.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxMctlGetIntNetNode(queue_t *q, mblk_t *mp)
{
	ipxMctl_t	*mctlp;
	uint32		*netp;
	uint8		*nodep;

	NTR_ENTER(2, q, mp, 0,0,0);
    NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
        "LipmxMctlGetIntNetNode: Enter"));

	if(DATA_SIZE(mp) != sizeof(ipxMctl_t)) {
		NWSLOG((LIPMXID,0,PNW_SWITCH_CASE,SL_TRACE,
			"LipmxMctlGetIntNetNode: M_CTL bad size %d ", DATA_SIZE(mp)));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}
	mctlp = (ipxMctl_t *)mp->b_rptr;
	netp = &mctlp->mctlblk.u_mctl.ipxInternalNetNode.net;
	nodep = mctlp->mctlblk.u_mctl.ipxInternalNetNode.node;
	LipmxCopyLanNet((uint32)0, netp);
	LipmxCopyLanNode((uint32)0, nodep);
	qreply(q, mp);
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIocStats(queue_t *q, mblk_t *mp)
 *	Return statistics to application.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxIocStats(queue_t *q, mblk_t *mp)
{	IpxLanStats_t *infop;
	IpxSocketStats_t *sinfop;

	NTR_ENTER(2, q, mp, 0,0,0);
    NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
        "LipmxIocStats: Enter"));

	if ((mp->b_cont == NULL) 
			|| (DATA_SIZE(mp->b_cont) < 
				sizeof(IpxLanStats_t) + sizeof(IpxSocketStats_t))) {
		NWSLOG((LIPMXID,0,PNW_ERROR, SL_TRACE,
			"LipmxIocStats: NULL/mis-sized b_cont for %d bytes",
			sizeof(IpxLanStats_t)));
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	infop = (IpxLanStats_t *)mp->b_cont->b_rptr;
	*infop = lipmxStats;

	/*
	**	If no socket mux, zero those statistics
	*/
	if( lipmxStats.IpxSocketMux == 0) {
		sinfop = (IpxSocketStats_t*)(mp->b_cont->b_rptr + sizeof(IpxLanStats_t));
		bzero( (char *)sinfop, sizeof( IpxSocketStats_t));
		
	}

	LipmxIocAck(q,mp,sizeof(IpxLanStats_t) + sizeof(IpxSocketStats_t));
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIocGetConfiguredLans(queue_t *q, mblk_t *mp)
 *	Return the current number of configured lans.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxIocGetConfiguredLans(queue_t *q, mblk_t *mp)
{	IpxConfiguredLans_t *maxLans;

	NTR_ENTER(2, q, mp, 0,0,0);
    NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
        "LipmxIocGetConfiguredLans: Enter"));

	if ((mp->b_cont == NULL) 
			|| (DATA_SIZE(mp->b_cont)
					< sizeof(IpxConfiguredLans_t))) {
		NWSLOG((LIPMXID,0,PNW_ERROR, SL_TRACE,
			"LipmxIocGetConfiguredLans: NULL/mis-sized b_cont for %d bytes",
			sizeof(IpxConfiguredLans_t)));
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	maxLans = (IpxConfiguredLans_t *)mp->b_cont->b_rptr;
	maxLans->lans = LipmxGetConfiguredLans();
	maxLans->maxHops = LipmxGetMaxHops();
	LipmxIocAck(q,mp,sizeof(IpxConfiguredLans_t));
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIocSetLanInfo(queue_t *q, mblk_t *mp)
 *	Set IPX network/node address and DataLink Layer information for a lan.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxIocSetLanInfo(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);
    NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
        "LipmxIocSetLanInfo: Enter"));
	
    /* ioctl root only usage */
    if (!IsPriv(q)) {
        NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
            "LipmxIocSetLanInfo: non root access attempted"));
        LipmxIocNegAck(q,mp,EPERM);
        NTR_VLEAVE();
        return;
    }

	if ((mp->b_cont == NULL) 
			|| (DATA_SIZE(mp->b_cont) < sizeof(lanInfo_t))) {
		NWSLOG((LIPMXID,0,PNW_ERROR, SL_TRACE,
			"LipmxIocSetLanInfo: NULL/mis-sized b_cont for %d bytes",
			sizeof(lanInfo_t)));
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	if (LipmxSetLanInfo((lanInfo_t *)mp->b_cont->b_rptr)) 
		LipmxIocAck(q,mp,0);
	else
		LipmxIocNegAck(q,mp,EINVAL);

	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIocLink(queue_t *q, mblk_t *mp)
 *	Link a lower lan driver to lipmx.
 *
 * Calling/Exit State:
 *	 No locks on entry or exit.
 *	
 */
void
LipmxIocLink(queue_t *q, mblk_t *mp)
{
	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
		"LipmxIocLink: SWITCH M_IOCTL: I_LINK"));

	/*
	**	only accept I_LINK ioctl command on dev 0, (also implies root)
	*/
	if (!IsControl(q)) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxIocLink: Attempt to link. Not minor dev 0"));
		LipmxIocNegAck(q,mp,EPERM);
		NTR_VLEAVE();
		return;
	}

#ifdef DEBUG
/* Only check size on debug, size guaranteed by streamhead
 */
	if ((mp->b_cont == NULL) 
			|| (DATA_SIZE(mp->b_cont) < sizeof(struct linkblk))) {
		NWSLOG((LIPMXID,0,PNW_ERROR, SL_TRACE,
			"LipmxIocLink: NULL/mis-sized b_cont for %d bytes",
			sizeof(struct linkblk)));
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
#endif /* def DEBUG */

	if (!LipmxLinkLan((struct linkblk *) mp->b_cont->b_rptr)) {
		LipmxIocNegAck(q,mp,EINVAL);
	} else {
		/*
	 	** driver is linked in -- send an ACK
	 	*/
		LipmxIocAck(q,mp,0);
	}

	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIocGetNetAddr(queue_t *q, mblk_t *mp)
 *	Return to the caller the IPX net address of the internal net.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxIocGetNetAddr(queue_t *q, mblk_t *mp)
{	IpxNetAddr_t	*net0;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LipmxIocGetNodeAddr"));

	if ((mp->b_cont == NULL) 
			|| (DATA_SIZE(mp->b_cont) < sizeof(IpxNetAddr_t))) {
		NWSLOG((LIPMXID,0,PNW_ERROR, SL_TRACE,
			"LipmxIocGetNodeAddr: NULL/mis-sized b_cont for %d bytes",
			sizeof(IpxNetAddr_t)));
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	net0 = (IpxNetAddr_t *)mp->b_cont->b_rptr;

	if (LipmxCopyLanNet(0, (uint32 *)net0->myNetAddress.net) == 0)
		LipmxIocAck(q,mp,sizeof(IpxNetAddr_t));
	else
		LipmxIocNegAck(q,mp,EINVAL);
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIocGetNodeAddr(queue_t *q, mblk_t *mp)
 *	Return to the caller the IPX node address of the internal net.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxIocGetNodeAddr(queue_t *q, mblk_t *mp)
{	IpxNodeAddr_t	*node0;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LipmxIocGetNodeAddr"));

	if ((mp->b_cont == NULL) 
			|| (DATA_SIZE(mp->b_cont) < sizeof(IpxNodeAddr_t))) {
		NWSLOG((LIPMXID,0,PNW_ERROR, SL_TRACE,
			"LipmxIocGetNodeAddr: NULL/mis-sized b_cont for %d bytes",
			sizeof(IpxNodeAddr_t)));
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	node0 = (IpxNodeAddr_t *)mp->b_cont->b_rptr;

	if (LipmxCopyLanNode(0, node0->myNodeAddress.node) == 0)
		LipmxIocAck(q,mp,sizeof(IpxNodeAddr_t));
	else
		LipmxIocNegAck(q,mp,EINVAL);
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIocGetLanInfo(queue_t *q, mblk_t *mp)
 *	Return information about a lan:
 *		IPX network/node address
 *		DataLink Layer information for a lan.
 *		state 
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxIocGetLanInfo(queue_t *q, mblk_t *mp)
{	lanInfo_t	*lanInfo;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LipmxIocGetLanInfo"));

	if (!mp->b_cont || (DATA_SIZE(mp->b_cont) != sizeof(lanInfo_t))) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxIocGetLanInfo: Bad data Size - expecting %d",
				sizeof(lanInfo_t)));
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}

	lanInfo = (lanInfo_t *)mp->b_cont->b_rptr;

	if (!LipmxGetLanInfo(lanInfo))
		LipmxIocNegAck(q,mp,EINVAL);
	else
		LipmxIocAck(q,mp,sizeof(lanInfo_t));

	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIocUnlink(queue_t *q, mblk_t *mp)
 *	LipmxIocUnlink should set the state of the network being unlinked and
 *	clear all elements of the private data structure. 
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxIocUnlink(queue_t *q, mblk_t *mp)
{	struct linkblk 	*linkp;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,"Enter IpxIocUnlink"));

#ifdef DEBUG
/* Only check size on debug, size guaranteed by streamhead
 */
	if (DATA_SIZE(mp->b_cont) < sizeof(struct linkblk)) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxIocUnlink: no link index"));
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
#endif /* def DEBUG */
	linkp = (struct linkblk *) mp->b_cont->b_rptr;

	NWSLOG((LIPMXID,0,PNW_ASSIGNMENT,SL_TRACE,
		"LipmxIocUnlink: link index = %x",linkp->l_index));

	if (LipmxUnlinkLan(linkp->l_index) < 0) {
#ifdef DEBUG
         cmn_err(CE_WARN,
  			"LipmxIocUnlink: UnLink problems !!");
#endif /* def DEBUG */
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
			"LipmxIocUnlink: MuxId not found"));
		LipmxIocNegAck(q,mp,EINVAL);
	} else {
		LipmxIocAck(q,mp,0);
	}

	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIocSetConfiguredLans(queue_t *q, mblk_t *mp)
 *	Set the number of lans that is going tobe configured by SetLanInfo
 *	Set maximum Hops.  Lipmx will discard any incoming packets with the 
 *	tc field greater than max hops. 
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxIocSetConfiguredLans(queue_t *q, mblk_t *mp)
{	IpxConfiguredLans_t *maxLans;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"LipmxIocSetConfiguredLans"));

    /* ioctl root only usage */
    if (!IsPriv(q)) {
        NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
            "LipmxIocSetConfiguredLans: non root access attempted"));
        LipmxIocNegAck(q,mp,EPERM);
        NTR_VLEAVE();
        return;
    }

	if (!mp->b_cont ||
			(DATA_SIZE(mp->b_cont) != sizeof(IpxConfiguredLans_t))) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxIocSetConfiguredLans: Bad data Size"));
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	maxLans = (IpxConfiguredLans_t *)mp->b_cont->b_rptr;

	if (!LipmxSetConfiguredLans(maxLans->lans)) {
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	LipmxSetMaxHops(maxLans->maxHops);
	NWSLOG((LIPMXID,0,PNW_ALL,SL_TRACE,
		"LipmxIocSetConfiguredLans: Setting ipxMaxHops to %d",
		maxLans->maxHops));

	LipmxIocAck(q,mp,0);
	NTR_VLEAVE();
	return;
}

/*
 * void LipmxIocSetSapQ(queue_t *q, mblk_t *mp)
 *	Function is only valid if ipxr is above liipmx.
 *	Set the upper read queue for sap packets.
 *	LIPMXSAP_LOCK (sapQLock) is allocated by ipxr only.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	Function acquires/releases LIPMXSAP_LOCK.
 *	
 */
void
LipmxIocSetSapQ(queue_t *q, mblk_t *mp)
{
	pl_t	lvl;

	NTR_ENTER(2, q, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LipmxIocSetSapQ"));

    if (!IsPriv(q)) {
        NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
            "LipmxIocSetSapQ: non-root access attempted"));
        LipmxIocNegAck(q,mp,EPERM);
        NTR_VLEAVE();
        return;
    }
	/* Invalid if under ipx */
	if (lipmxStats.IpxSocketMux) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxIocSetSapQ: Invalid, LIPMX is under IPX"));
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	ASSERT(sapQLock != NULL);
	lvl = LOCK(sapQLock, plstr);
	if (sapQ) {
		NWSLOG((LIPMXID,0,PNW_ERROR,SL_TRACE,
				"LipmxIocSetSapQ: Already Set"));
		UNLOCK(sapQLock, lvl);
		LipmxIocNegAck(q,mp,EINVAL);
		NTR_VLEAVE();
		return;
	}
	sapQ = q;
	ATOMIC_INT_INIT( &sapQcount, 0);
	UNLOCK(sapQLock, lvl);
	LipmxIocAck(q,mp,0);
	NTR_VLEAVE();
	return;
}

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

#ident	"@(#)kern:net/nw/spx/spx2rd.c	1.20"
#ident	"$Id: spx2rd.c,v 1.21.2.4 1994/11/16 17:14:47 meb Exp $"

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
#include <net/nw/spx/spx2.h>
#else
#include "spx2.h"
#endif


/*
 * long SpxSendAck(uint16 conId, int overRideSeq, uint16 seqNumber)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
long
SpxSendAck(uint16 conId, int overRideSeq, uint16 seqNumber)
{
	register spxConnectionEntry_t *conEntryPtr;
	register spxHdr_t *outSpxPac;

	mblk_t	*spxAckMp;
	long	saError;
	uint32	llocAck, llocAlloc;

	NTR_ENTER( 3, conId, overRideSeq, seqNumber, 0, 0);
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"NSPX: ENTER SendAck"));

	saError = 0;

	if (conId >= spxMaxConnections) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: SendAck : conId invalid %d",conId));
		saError = EINVAL;
		goto Exit;
	}

	conEntryPtr = &spxConnectionTable[conId];

	if (spxbot == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: SendAck : spxbot is NULL"));
		saError = ENOLINK;
		goto Exit;
	}
		
	if (!canputnext(spxbot)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: SendAck : cannot put down to q %Xh",spxbot));
		conEntryPtr->conStats.con_canput_fail++; 
		saError = ENOSTR;
		goto Exit;
	}

	conEntryPtr->spxHdr.connectionControl = conEntryPtr->protocol; 

	if ((spxAckMp = SpxSetUpPacket(conId)) == NULL ) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: SendAck : SetUpPacket failed"));
		saError = ENOSR;
		goto Exit;
	}

	outSpxPac = (spxHdr_t *)spxAckMp->b_rptr;
	outSpxPac->connectionControl |= SPX_SYSTEM_PACKET;
	outSpxPac->ipxHdr.len = GETINT16( msgdsize(spxAckMp) );

	/*		Only Native SPX looks at Sequence Number on a ACK!!!!
	 * In some error cases the sequence number in conEntryPtr is
	 * one beyond what the other end is expecting.  We give him
	 * what he is expecting, instead of what our internal structure
	 * says it should be.
	 */
	if (overRideSeq) {   
		outSpxPac->sequenceNumber = GETINT16(seqNumber);  
	}
	if (conEntryPtr->protocol & SPX_SPXII ) {
		outSpxPac->sequenceNumber = (uint16)0;
	}

	/*
	 * Make long values for comparisons and adjust to handle possible
	 * wraparound, otherwise we will never reopen the window.
	 */
	llocAck = conEntryPtr->window.local.ack;
	if ((conEntryPtr->window.local.ack > 0xff80 ) &&
				(conEntryPtr->window.local.alloc < 0x0080))  {
		llocAlloc = conEntryPtr->window.local.alloc + 0x10000;
	}
	else {
		llocAlloc = conEntryPtr->window.local.alloc;
	}

	if ((conEntryPtr->window.local.windowClosed ) && 
		(llocAlloc >= llocAck)) {
		/* If window is being reopened, start a timer to Ping the connection,
		   in case this acknowledgment is lost.
		*/
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: lrp: SendAck: ReOpening Window, Start Ack timeout"));
		/* set timeout for 3 sec */
		conEntryPtr->aTimeOutId = itimeout(SpxAckTimeOut, (void *)conId,
			(3 * HZ), pltimeout);
	}
	if (llocAlloc < llocAck) {
		conEntryPtr->window.local.windowClosed = TRUE; 
	} else {
		conEntryPtr->window.local.windowClosed = FALSE;
	}

	conEntryPtr->conStats.con_send_ack++; 
	putnext(spxbot,spxAckMp);

	Exit:

		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: SendAck : EXIT "));

		return( NTR_LEAVE(saError));
}

/*
 * long SpxSendNak(uint16 conId, uint16 numberMissed)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
long
SpxSendNak(uint16 conId, uint16 numberMissed)
{
	register spxConnectionEntry_t *conEntryPtr;
	register spxHdr_t *outSpxPac;
	mblk_t *spxNakMp;
	long saError;

	NTR_ENTER( 2, conId, numberMissed, 0, 0, 0);
	NWSLOG((SPXID, conId, PNW_ENTER_ROUTINE, SL_TRACE,
			"NSPX: ENTER SendNak"));

	saError = 0;

	if (conId >= spxMaxConnections) {
		NWSLOG((SPXID, conId, PNW_ERROR, SL_TRACE, 
			"NSPX: SendNak : conId invalid %d",conId));
		saError = EINVAL;
		goto Exit;
	}

	conEntryPtr = &spxConnectionTable[conId];

	if (spxbot == NULL) {
		NWSLOG((SPXID, conId, PNW_ERROR, SL_TRACE, 
			"NSPX: SendNak : spxbot is NULL"));
		saError = ENOLINK;
		goto Exit;
	}

	if (!canputnext(spxbot)) {
		NWSLOG((SPXID, conId, PNW_ERROR, SL_TRACE, 
			"NSPX: SendNak : cannot put down to q %Xh",spxbot));
		conEntryPtr->conStats.con_canput_fail++; 
		saError = ENOSTR;
		goto Exit;
	}

	conEntryPtr->spxHdr.connectionControl = conEntryPtr->protocol; 

	if ((spxNakMp = SpxSetUpPacket(conId)) == NULL ) {
		NWSLOG((SPXID, conId, PNW_ERROR, SL_TRACE, 
			"NSPX: SendNak : SetUpPacket failed"));
		saError = ENOSR;
		goto Exit;
	}

	outSpxPac = (spxHdr_t *)spxNakMp->b_rptr;
	outSpxPac->connectionControl |= SPX_SYSTEM_PACKET;
	outSpxPac->ipxHdr.len = GETINT16( msgdsize(spxNakMp) );
	outSpxPac->sequenceNumber = GETINT16(numberMissed);  
	NWSLOG((SPXID, conId, PNW_ERROR, SL_TRACE, 
		"NSPX: SendNak: Missed %d starting with sequence number %d"
		,numberMissed,GETINT16(outSpxPac->acknowledgeNumber)));
	conEntryPtr->conStats.con_send_nak++; 
	putnext(spxbot,spxNakMp);

	Exit:
		NWSLOG((SPXID, conId, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: SendNak : EXIT "));

		return( NTR_LEAVE(saError));
}

/*
 * void SpxConnReq(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
/*ARGSUSED*/
void 
SpxConnReq(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;
	spxHdr_t *spxPacHdr;
	ipxMctl_t *spxMctlPtr;	
	register uint i,j; 
	uint	 index;

	struct T_conn_ind *tConnInd;
	spxUnitHdr_t *spxUnit;
	SPX2_OPTIONS *spx2OptRet;
	queue_t *upperQueue;
	uint16 desSock;
	uint32 netwrk;
	uint tConnIndSize;
	mblk_t *nmp;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
/* size has already been checked by spxlrput */
	spxPacHdr = (spxHdr_t *)mp->b_rptr;

	IPXCOPYSOCK(spxPacHdr->ipxHdr.dest.sock, &desSock);

	NWSLOG((SPXID, spxMinorDev, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: SpxConnReq: ENTER "));

/* see if the destination socket has been bound by someone in userland
*/
	for (i=0; i < spxSocketCount; i++) 
		if (spxSocketTable[i].socketNumber == desSock) 
				break;

	if ( i >= spxSocketCount ) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: SpxConnReq: none bound to socket %Xhn",
			GETINT16((uint)desSock)));
	 	spxStats.spx_no_listeners++; 
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

/* see if listens were posted with the bind */
	if (spxSocketTable[i].postedListens <= (uint16)0) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: SpxConnReq: socket %Xhn bound with no listens ",
			(uint)desSock));
	 	spxStats.spx_no_listeners++; 
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

/* if perchance the connection ack failed then resend it */

	for (j=0; j<spxMaxConnections; j++) {
		if (spxConnectionTable[j].upperReadQueue == NULL) 
			continue;
		if (
			(IPXCMPNET(spxConnectionTable[j].spxHdr.ipxHdr.dest.net,
				spxPacHdr->ipxHdr.src.net)) &&
			(IPXCMPNODE(spxConnectionTable[j].spxHdr.ipxHdr.dest.node,
				spxPacHdr->ipxHdr.src.node)) &&
			(IPXCMPSOCK(spxConnectionTable[j].spxHdr.ipxHdr.dest.sock,
				spxPacHdr->ipxHdr.src.sock)) &&
			( spxConnectionTable[j].spxHdr.destinationConnectionId ==
				spxPacHdr->sourceConnectionId )
			) {
				NWSLOG((SPXID, spxMinorDev, PNW_DROP_PACKET, SL_TRACE,
					"NSPX: SpxConnReq: retry acking connection request "));
				freemsg(mp);
				SpxSendAck(j, FALSE, 0);
				NTR_VLEAVE();
				return;
			}
	}

/* compare this request to see if we already got it ie we are 
	waiting on userland */
	for (j=0; j<spxSocketTable[i].postedListens; j++) {
		if (spxSocketTable[i].listens[j].acked) 
			continue;

		if ((IPXCMPNET( spxSocketTable[i].listens[j].spxInfo.addr.net,
				spxPacHdr->ipxHdr.src.net)) &&
			(IPXCMPNODE( spxSocketTable[i].listens[j].spxInfo.addr.node,
				spxPacHdr->ipxHdr.src.node)) &&
			(IPXCMPSOCK( spxSocketTable[i].listens[j].spxInfo.addr.sock,
				spxPacHdr->ipxHdr.src.sock)) &&
			(spxSocketTable[i].listens[j].spxInfo.connectionId ==  
				spxPacHdr->sourceConnectionId)  
			) {
				NWSLOG((SPXID, spxMinorDev, PNW_DROP_PACKET, SL_TRACE,
					"NSPX: SpxConnReq: duplicate connection request "));
				freemsg(mp);
				NTR_VLEAVE();
				return;
			}
	}
/* find a slot in the socket table that hasn't been acked */
	for (j=0; j<spxSocketTable[i].postedListens; j++) 
		if (spxSocketTable[i].listens[j].acked) 
			break;

	if (j>=spxSocketTable[i].postedListens) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: SpxConnReq: no free posted listens"));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

/* check the state of the queue receiving the indication */
	index = spxSocketTable[i].listenConnectionId;
	index &= CONIDTOINDEX;
	if ((spxConnectionTable[index].state != TS_WRES_CIND)  && 
		(spxConnectionTable[index].state != TS_IDLE)) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: SpxConnReq: listening q %Xh invalid state %d",
			spxConnectionTable[index].upperReadQueue,
			spxConnectionTable[index].state)); 
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

/* save conn req info */
	upperQueue = spxConnectionTable[index].upperReadQueue;

	spxSocketTable[i].listens[j].spxInfo.addr = spxPacHdr->ipxHdr.src;

	spxSocketTable[i].listens[j].spxInfo.connectionId =
							spxPacHdr->sourceConnectionId;

	spxSocketTable[i].listens[j].spxInfo.allocationNumber =
								PGETINT16(&spxPacHdr->allocationNumber);

/* make sure we can change the spx packet into a t_conn_ind */
	conEntryPtr = &spxConnectionTable[index];
	if (conEntryPtr->spx2Options)
		tConnIndSize = sizeof(struct T_conn_ind) + sizeof(ipxAddr_t) 
			+ sizeof(SPX2_OPTIONS);
	else
		tConnIndSize = sizeof(struct T_conn_ind) + sizeof(ipxAddr_t) 
			+ sizeof(SPX_OPTS);

	conEntryPtr->protocol &= spxPacHdr->connectionControl;
/* get net # for M_CTL */
	IPXCOPYNET(spxPacHdr->ipxHdr.src.net, &netwrk);

/* this listen slot isnt taken till we set acked to false
	so if the alloc fails we just let the client try again
	then maybe a buf will be available*/

#ifdef OS_SUN5  /* sun kludge   db_base may not be aligned in SUN 5.X */
	mp->b_rptr = (uint8 *)((((long )mp->b_datap->db_base +3) >> 2) << 2);
#else
	mp->b_rptr = mp->b_datap->db_base;
#endif

	if (DATA_SIZE(mp) < tConnIndSize) {
		NWSLOG((SPXID, spxMinorDev, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: SpxConnReq: have to alloc conn_ind msg %d",
			tConnIndSize));
		freemsg(mp);
		if ((mp = allocb(tConnIndSize, BPRI_MED)) == NULL) {
			spxStats.spx_alloc_failures++; 
			NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
				"NSPX: SpxConnReq: couldnt alloc msg size %d",
				tConnIndSize));
			NTR_VLEAVE();
			return;
		}
	}

	if ((nmp = allocb(sizeof(ipxMctl_t), BPRI_HI)) == NULL) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: SpxConnReq: cant alloc %d ",
		sizeof(ipxMctl_t)));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	spxSocketTable[i].listens[j].acked = FALSE;

/* build and send up t_conn_ind */
	MTYPE(mp)=M_PROTO;
	mp->b_wptr= mp->b_rptr + tConnIndSize;

	tConnInd = (struct T_conn_ind *)mp->b_rptr;	
	spxUnit = (spxUnitHdr_t *)(mp->b_rptr + sizeof(struct T_conn_ind));

	tConnInd->PRIM_type = T_CONN_IND;
	tConnInd->SRC_length = sizeof(ipxAddr_t);
	tConnInd->SRC_offset = sizeof(struct T_conn_ind);
	tConnInd->OPT_offset = sizeof(struct T_conn_ind)+ sizeof(ipxAddr_t);
	tConnInd->SEQ_number = j;
	
	if (conEntryPtr->spx2Options) {
		tConnInd->OPT_length = sizeof(SPX2_OPTIONS);
		bcopy((char *)&spxSocketTable[i].listens[j].spxInfo, (char *)spxUnit,
			sizeof(ipxAddr_t));
		spx2OptRet = (SPX2_OPTIONS *)
		(mp->b_rptr + sizeof(struct T_conn_ind) + sizeof(ipxAddr_t));

		spx2OptRet->versionNumber = OPTIONS_VERSION;
		if (conEntryPtr->protocol & SPX_NEG)
			spx2OptRet->spxIIOptionNegotiate = SPX2_NEGOTIATE_OPTIONS;
		else {
			spx2OptRet->spxIIOptionNegotiate = SPX2_NO_NEGOTIATE_OPTIONS;
			/* if No negotiate, can not have checksums.*/
			conEntryPtr->ipxChecksum = FALSE;
		}
		spx2OptRet->spxIIRetryCount = conEntryPtr->tMaxRetries;
		spx2OptRet->spxIIMinimumRetryDelay = 
			TCKS2MSEC(conEntryPtr->minTicksToWait);
		spx2OptRet->spxIIMaximumRetryDelta = 
			TCKS2MSEC(conEntryPtr->maxTicksToWait);
		spx2OptRet->spxIILocalWindowSize = conEntryPtr->spxWinSize;
		spx2OptRet->spxIIWatchdogTimeout = TCKS2MSEC(spxWatchEmuInterval);
		spx2OptRet->spxIIConnectionTimeout = 0;
		spx2OptRet->spxIIRemoteWindowSize =
			spxSocketTable[i].listens[j].spxInfo.allocationNumber;
		spx2OptRet->spxIIConnectionID = 
			spxSocketTable[i].listens[j].spxInfo.connectionId;
		spx2OptRet->spxIIInboundPacketSize = conEntryPtr->maxRPacketSize;
		spx2OptRet->spxIIOutboundPacketSize = conEntryPtr->maxTPacketSize;
		spx2OptRet->spxIISessionFlags = SPX_SF_NONE;
		if (conEntryPtr->ipxChecksum)
			spx2OptRet->spxIISessionFlags |= SPX_SF_IPX_CHECKSUM;
		if (conEntryPtr->protocol & SPX_SPXII)
			spx2OptRet->spxIISessionFlags |= SPX_SF_SPX2_SESSION;
	} else {
		tConnInd->OPT_length = sizeof(SPX_OPTS);
		bcopy((char *)&spxSocketTable[i].listens[j].spxInfo, (char *)spxUnit,
			sizeof(spxUnitHdr_t));
	}

	spxConnectionTable[index].state = TS_WRES_CIND;

	if (!canputnext(upperQueue)) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: SpxConnReq: cant put up to %Xh",
			upperQueue));
		freemsg(mp);
		freemsg(nmp);
		NTR_VLEAVE();
		return;
	}

	noenable(WR(upperQueue)); /* disable WR queue until MCTL is done */
	conEntryPtr->disabled = TRUE;
	putnext(upperQueue,mp);

	if (conEntryPtr->protocol == (SPX_SPXII | SPX_NEG)) {
		MTYPE(nmp) = M_CTL;
		nmp->b_wptr = nmp->b_rptr + sizeof(ipxMctl_t); 
		spxMctlPtr = (ipxMctl_t *)nmp->b_rptr;	
		spxMctlPtr->mctlblk.cmd = SPX_GET_IPX_MAX_SDU; 
		spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.network = netwrk; 
		spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.maxSDU = 0; 
		spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.conEntry = 
					(caddr_t)conEntryPtr; 

		/* this is a priority message send it regardless of canput */
		putnext(spxbot,nmp);
	} else {
		conEntryPtr->sessionFlags = NO_NEGOTIATE;
		conEntryPtr->maxSDU = 
		conEntryPtr->maxTPacketSize =
		conEntryPtr->maxRPacketSize = SPX_DEFAULT_PACKET_SIZE;
		/* if No negotiate, can not have checksums.*/
		conEntryPtr->ipxChecksum = FALSE;
		freemsg(nmp);
		conEntryPtr->disabled = FALSE;
		enableok(WR(upperQueue));
		qenable(WR(upperQueue));
	}

	NWSLOG((SPXID, spxMinorDev, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: SpxConnReq: EXIT "));
	NTR_VLEAVE();
	return;
}

/*
 * void SpxReNegotiate(int conId)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxReNegotiate(int conId)
{
	register spxConnectionEntry_t *conEntryPtr;

	NTR_ENTER( 1, conId, 0, 0, 0, 0);
	NWSLOG((SPXID, conId, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: ENTER ReNegotiate")); 

	if (conId >= spxMaxConnections) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: ReNegotiate: bad connection Id %d",conId));
		NTR_VLEAVE();
		return;
	}

	conEntryPtr = &spxConnectionTable[conId];
	if (conEntryPtr->tTimeOutId) {
		conEntryPtr->tTimeOutId = 0;
	}
	if (conEntryPtr->pRetries >= conEntryPtr->tMaxRetries ) {
		NWSLOG((SPXID, conId, PNW_ERROR, SL_TRACE,
			"NSPX: ReNegotiate: reached max retries %d closing connection",
				conEntryPtr->tMaxRetries));
		spxStats.spx_max_retries_abort++;
		SpxGenTDis(conEntryPtr, (long) TLI_SPX_CONNECTION_FAILED, FALSE);
		NTR_VLEAVE();
		return;
	}

	conEntryPtr->tTicksToWait += (conEntryPtr->tTicksToWait >> 1);
	 
	if ((conEntryPtr->tTicksToWait > conEntryPtr->maxTicksToWait ) ||
			(conEntryPtr->tTicksToWait == 0))
		conEntryPtr->tTicksToWait = conEntryPtr->maxTicksToWait ;
 
	NWSLOG((SPXID, conId, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX: ReNegotiate: Ping Connection,  retry= %d",
		conEntryPtr->pRetries));
	conEntryPtr->pRetries++;
	conEntryPtr->tTimeOutId = itimeout(SpxReNegotiate, (void *)conId, 
		conEntryPtr->tTicksToWait, pltimeout); 

	SpxPing(conId);

	NWSLOG((SPXID, conId, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: EXIT ReNegotiate")); 
	NTR_VLEAVE();
	return;
}

/*
 * void SpxAck(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
/*ARGSUSED*/
void
SpxAck(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;
	register spxHdr_t *inspxHeader;
	spxIIHdr_t *inspxIIHeader;
	spxHdr_t *spxPacket; 
	struct 	T_ok_ack *tOkAck;
	mblk_t *spxCopy;
	uint16 	destConId;
	uint16 	spxAckNumber, spxAllocNumber;
	uint16 	packetsLost =0;
	uint16 	remoteAck, localSeq, retrySeq;
	uint32	lremoteAck, lspxAckNumber, lLocalSeq, lSpxAlloc, lRetrySeq;
	uint16 	i, ackMsg, unAckMsg, negotSize;
	struct T_data_req *tDataReq;
	long	moreFlag;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	inspxHeader = (spxHdr_t *)mp->b_rptr;
	destConId = PGETINT16(&inspxHeader->destinationConnectionId);
	destConId &= CONIDTOINDEX;
	conEntryPtr = &spxConnectionTable[destConId];

	NWSLOG((SPXID, destConId, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: lrp: ENTER GotAck"));	

	if (conEntryPtr->aTimeOutId) {
		untimeout(conEntryPtr->aTimeOutId);
		conEntryPtr->aTimeOutId = 0;
	}

	remoteAck = conEntryPtr->window.remote.ack;
	localSeq = conEntryPtr->window.local.sequence;
	retrySeq = conEntryPtr->window.local.retrySequence;
	spxAckNumber = PGETINT16(&inspxHeader->acknowledgeNumber);
	spxAllocNumber = PGETINT16(&inspxHeader->allocationNumber);
 
	/* convert spxAckNumber, spxAllocNumber and local.seq number to longs
	   to handle wrap of sequence number (0xffff to 0x0000).
	*/

	/* adjust lspxAckNumber for case:
	 *  remoteAck(last acknowledge # from other endpoint) 0xfffd
	 *  spxAckNumber(acknowledge # in this packet) 0x002
	 */
	if (((remoteAck > 0xff80) && (spxAckNumber < 0x0080 )) ||
			((retrySeq > 0xff80) && (spxAckNumber < 0x0080 )))
		lspxAckNumber = spxAckNumber + 0x10000;
	else
		lspxAckNumber = spxAckNumber;

	if ((lspxAckNumber > 0xff80) && (remoteAck < 0x0080 )) {
		lremoteAck = remoteAck + 0x10000;
	} else {
		lremoteAck = remoteAck;
	}

	/* adjust lLocalSeq for case:
	 *  remoteAck(last acknowledge # from other endpoint) 0xfffd
	 *  spxAckNumber(acknowledge # in this packet) 0x002
	 *  localSeq(the last packet we sent) 0x002
	 */
	if ((localSeq < 0x0080) && (lspxAckNumber > 0xff80))
		lLocalSeq = localSeq + 0x10000;
	else
		lLocalSeq = localSeq;

	if ((retrySeq < 0x0080) && (lspxAckNumber >= 0xff80))
		lRetrySeq = retrySeq + 0x10000;
	else
		lRetrySeq = retrySeq;

	if ((spxAllocNumber < 0x0080) && (lLocalSeq >= 0xff80))
		lSpxAlloc = (spxAllocNumber + 0x10000);
	else
		lSpxAlloc = spxAllocNumber;

	NWSLOG((SPXID, destConId, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX:Ack: seq=%d ack=%d alloc=%d ",
		PGETINT16(&inspxHeader->sequenceNumber),lspxAckNumber,lSpxAlloc));
	NWSLOG((SPXID, destConId, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX:Ack: remoteAck=%d localSeq=%d localRety=%d",
		lremoteAck,lLocalSeq,lRetrySeq));

	/* check if incoming acknowledge is within range, 
	   greater than or equal to the last ack received,
	   and less than or equal to the next sequenceNumber to send.
	*/
	if((lspxAckNumber >= lremoteAck) && (lspxAckNumber <= lLocalSeq)) {
		NWSLOG((SPXID, destConId, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: lrp: Ack: got ack for packets below seq %d alloc= %d",
			spxAckNumber,spxAllocNumber));

		/* remove all packets acknowledge from the unAckedMsgs list.  */
		for(i=0; i < (lspxAckNumber - lremoteAck); i++) {
			ackMsg = ((uint16)(remoteAck + i) % MAX_SPX2_WINDOW_SIZE);
			if (conEntryPtr->window.unAckedMsgs[ackMsg] != NULL) {
				if (conEntryPtr->waitOnDataMp) 
					if (((spxHdr_t *)(conEntryPtr-> 
						window.unAckedMsgs[ackMsg]->b_rptr))-> 
						connectionControl & (uint8)SPX_EOF_BIT) {
  
						putnext(conEntryPtr->upperReadQueue, 
							conEntryPtr->waitOnDataMp);	 
						conEntryPtr->waitOnDataMp = NULL; 
					} 
				freemsg(conEntryPtr->window.unAckedMsgs[ackMsg]); 
				conEntryPtr->window.unAckedMsgs[ackMsg] = NULL; 
			} 
		} 

		/* if SPXII and a sequence number, this is a NAK.
		   Resend the # of packets requested (sequence number) starting
		   at the acknowledge number.
		*/
		if (inspxHeader->connectionControl & SPX_SPXII)
			packetsLost = PGETINT16(&inspxHeader->sequenceNumber);
		if (packetsLost)
			conEntryPtr->conStats.con_rcv_nak++; 
		else
			conEntryPtr->conStats.con_rcv_ack++; 

		/* since the Orderly Release Ack was removed from the spec, 
		   check to see if we have sent Orderly Release Requeset
		   and if it is being acknowledged.  If so change state.
		*/
		if(conEntryPtr->ordRelReqSent) {
			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:Ack: OrdRel sent ordSeqNumber= %d lspxAckNumber=%d ",
				conEntryPtr->ordSeqNumber, lspxAckNumber));
			if (lspxAckNumber > conEntryPtr->ordSeqNumber) {
				if ( conEntryPtr->state == TS_WREQ_ORDREL ) {
					conEntryPtr->state = TS_IDLE;
				} else {
					conEntryPtr->state = TS_WIND_ORDREL;
				}
				NWSLOG((SPXID, destConId, PNW_ASSIGNMENT, SL_TRACE,
					"NSPX:Ack: got ack for OrdRel seq# %d, change state to %d",
					conEntryPtr->ordSeqNumber, conEntryPtr->state));
				conEntryPtr->ordRelReqSent = 0; 
			}
		}

		/* update values only if acknowledging last packet sent*/
		if(lspxAckNumber > lRetrySeq) {
			if (conEntryPtr->tTimeOutId) {
				NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE,
					"Ack: Cancel TxTimeout ID=%x lspxAck#= %d lRetrySeq= %d",
					conEntryPtr->tTimeOutId, lspxAckNumber, lRetrySeq));
				untimeout(conEntryPtr->tTimeOutId);
				conEntryPtr->tTimeOutId = 0;
			}
			conEntryPtr->needAck = FALSE; 
			conEntryPtr->tRetries = 0; 
		}
		/* update remoteAck with the acknowlegde just recieved. */ 
		conEntryPtr->window.remote.ack = spxAckNumber; 
	}

	/* check if this is a SIZE Negotiation ACK. If so, save negotation size
	   this will be the maximum transmit packet Size
	*/
	if ((inspxHeader->connectionControl & SPX_SPXII) &&
		(conEntryPtr->protocol & SPX_SPXII) &&
		((conEntryPtr->sessionFlags & WAIT_NEGOTIATE_ACK) ||
		(inspxHeader->connectionControl & SPX_NEG))) {

		if (conEntryPtr->tTimeOutId) {
			untimeout(conEntryPtr->tTimeOutId);
			conEntryPtr->tTimeOutId = 0;
		}
		conEntryPtr->sessionFlags &= ~WAIT_NEGOTIATE_ACK;

		inspxIIHeader = (spxIIHdr_t *)mp->b_rptr;
		if (inspxIIHeader->connectionControl & SPX_NEG) {
			negotSize = PGETINT16(&inspxIIHeader->negotiateSize);
			if (negotSize < (uint16)SPX_DEFAULT_PACKET_SIZE)
				negotSize = SPX_DEFAULT_PACKET_SIZE;

			conEntryPtr->maxTPacketSize = negotSize;
		}
		NWSLOG((SPXID, destConId, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: lrp: Ack: Negotiation Ack, Size=%x", negotSize));

		if (conEntryPtr->sessionFlags & SESS_SETUP) {
			conEntryPtr->sessionFlags &= ~SESS_SETUP;
			/* Clear NEG Bit Negotiation Done*/
			conEntryPtr->protocol &= ~SPX_NEG; 
			conEntryPtr->state = TS_DATA_XFER;
			if (conEntryPtr->ipxChecksum == TRUE)
				conEntryPtr->spxHdr.ipxHdr.chksum = IPX_CHKSUM_TRIGGER;
			MTYPE(mp) = M_PCPROTO;
			tOkAck = (struct T_ok_ack *)mp->b_rptr;
			tOkAck->PRIM_type = T_OK_ACK;
			tOkAck->CORRECT_prim = T_CONN_RES;
			mp->b_wptr = mp->b_rptr+sizeof(struct T_ok_ack);
			if (mp->b_cont) {
				freemsg(mp->b_cont);
				mp->b_cont = 0;
			}
		/* send T_OK_ACK up on the same queue that T_CONN_RES came down on*/
			qreply(conEntryPtr->responseQueue,mp);
			conEntryPtr->responseQueue = NULL;
			NWSLOG((SPXID, destConId, PNW_EXIT_ROUTINE, SL_TRACE,
					"NSPX: lrp: EXIT Got Session SetUp Ack"));	
			NTR_VLEAVE();
			return;
		} else { 
			/* Must be Neg ACK or ReNeg ACK or possibly an
			*  Negotiation ACK to multiple Negotiation Requests
			*  (Session setup Req, ReNegotiation Req) due to the
			*  first Neg ACK being delayed. */
			if (conEntryPtr->sessionFlags & RE_NEGOTIATE) {
				conEntryPtr->sessionFlags &= ~RE_NEGOTIATE;
			}
			packetsLost = lLocalSeq - lspxAckNumber;
		
			/* The maxTPacketSize might of been reduced, and if there is
			 * unacknowledge packets they will need NEW sequence numbers.
			 * Put all unAcked msgs back on the WR queue.
			 */
			if (packetsLost)	{
				for(i=1; i <= MAX_SPX2_WINDOW_SIZE; i++) {
					unAckMsg = ((uint16)(spxAckNumber - i) %
						MAX_SPX2_WINDOW_SIZE);
					if (conEntryPtr->window.unAckedMsgs[unAckMsg] == NULL)
						continue;
					spxCopy = conEntryPtr->window.unAckedMsgs[unAckMsg];
					spxPacket = (spxHdr_t *)spxCopy->b_rptr; 
					if (spxPacket->connectionControl & (uint8) SPX_EOF_BIT)
						moreFlag = FALSE;
					else
						moreFlag = TRUE;
					spxCopy->b_wptr = spxCopy->b_rptr + 
						sizeof(struct T_data_req);
					MTYPE(spxCopy) = M_PROTO;
					tDataReq = (struct T_data_req *)spxCopy->b_rptr;
					tDataReq->PRIM_type = T_DATA_REQ;
					tDataReq->MORE_flag = moreFlag;
					putbq(WR(conEntryPtr->upperReadQueue),spxCopy);
					conEntryPtr->window.unAckedMsgs[unAckMsg] = NULL;
				}
				packetsLost = 0;
			}
			conEntryPtr->disabled = FALSE;
			enableok(WR(conEntryPtr->upperReadQueue));
			conEntryPtr->needAck = FALSE; /* make sure we qenable */
		}
	}
	/* This code will be executed on the Ack from the Ping after finding
	   new route to endpoint. Send ReNegotiation Req   */
	if (conEntryPtr->sessionFlags & RE_NEGOTIATE) {
		if (conEntryPtr->tTimeOutId) {
			untimeout(conEntryPtr->tTimeOutId);
			conEntryPtr->tTimeOutId = 0;
		}
		if ((conEntryPtr->sessionFlags & NO_NEGOTIATE) || 
			(!(conEntryPtr->protocol & SPX_SPXII))) {
			conEntryPtr->maxTPacketSize = SPX_DEFAULT_PACKET_SIZE;
			packetsLost = lLocalSeq - lspxAckNumber;
			conEntryPtr->sessionFlags &= ~RE_NEGOTIATE;
			conEntryPtr->disabled = FALSE;
			enableok(WR(conEntryPtr->upperReadQueue));
			conEntryPtr->tTimeOutId = itimeout(SpxTTimeOut,
				(void *)destConId, conEntryPtr->tTicksToWait, pltimeout);
			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_ERROR,
				"NSPX:ACK: RE-NEG Schedule TTimeOut ID=%x, NO_NEG flag set",
				conEntryPtr->tTimeOutId));
		} else {
			conEntryPtr->sizeNegoRetry = 0;
			SpxSendNegSessPkt(conEntryPtr);
		}
	}
	if (packetsLost) {
		NWSLOG((SPXID, destConId, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: lrp: Ack: resend %d packets starting at seq %d",
			packetsLost,spxAckNumber));
		/*
		 * Do not Cancel Timeout, we will keep timer/retrying the last
		 * packet sent.  If a NAK is sent in the middle of a window
		 * (Native 4.02), we will lose the timer on the last packet sent.
		 * and could hang.
		 */
		for(i=0; i < packetsLost; i++) {
			NWSLOG((SPXID, destConId, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX: lrp: Ack: resending sequence Number %d",
				(spxAckNumber + i)));

			if (!canputnext(spxbot)) {
				conEntryPtr->conStats.con_canput_fail++; 
				NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
					"NSPX: SpxAck: cant put down to q %Xh",spxbot));
				break;
			}
			unAckMsg = ((uint16)(spxAckNumber + i) %
				MAX_SPX2_WINDOW_SIZE);
			if (conEntryPtr->window.unAckedMsgs[unAckMsg] == NULL)
				continue;

			if ((spxCopy = 
				dupmsg(conEntryPtr->window.unAckedMsgs[unAckMsg]))==NULL) {
				NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
					"NSPX: lrp: Ack: (dupmsg) cant alloc %d",
					msgdsize(conEntryPtr->window.unAckedMsgs[unAckMsg])));
				spxStats.spx_alloc_failures++; 
				break;
			}
			spxPacket = (spxHdr_t *)spxCopy->b_rptr; 
			spxPacket->acknowledgeNumber = 
				GETINT16(conEntryPtr->window.local.ack);  
			spxPacket->allocationNumber = 
				GETINT16(conEntryPtr->window.local.alloc);  
			conEntryPtr->window.local.lastAdvertisedAlloc =
				conEntryPtr->window.local.alloc;  
			/* this is a duplicated (dupmsg) packet that is being re-sent,
			 * IPX modified the checksum field on the original packet which 
			 * changed this checksum field also. If checksums are enabled 
			 * put in the IPX Trigger value so Ipx will generate a checksum.
			 */
			if ((conEntryPtr->ipxChecksum == TRUE) && 
				((conEntryPtr->state == TS_DATA_XFER) ||
				(conEntryPtr->state == TS_WREQ_ORDREL) ||
				(conEntryPtr->state == TS_WIND_ORDREL))) {
						spxPacket->ipxHdr.chksum = IPX_CHKSUM_TRIGGER;
			}
			/* if last packet to send, set Ack bit and needAck 
			** and schedule timeout */
			if ( i == (packetsLost - 1) ) {
				spxPacket->connectionControl |= SPX_ACK_REQUESTED;
				/*
				 * Do not schedule a timout for retry packets,
				 * keep orignal timer running, but reset ticks to wait and
				 * retry counter.
				 */
				conEntryPtr->tTicksToWait = conEntryPtr->minTicksToWait;
				conEntryPtr->tRetries = 0; 
			}
			spxStats.spx_send_packet_nak++; 
			conEntryPtr->conStats.con_send_packet_nak++; 
			putnext(spxbot,spxCopy);
		}
	}

	/*
	 * if the next packet sequence number is less than or equal to the new
	 * allocation number, send some more data (qenable).
	 */
	if ((lLocalSeq <= lSpxAlloc ) && (conEntryPtr->needAck == FALSE))  {
		conEntryPtr->window.remote.alloc = spxAllocNumber;
		if (!conEntryPtr->disabled) {
			NWSLOG((SPXID, destConId, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX: lrp: Ack: window is opened qenabling %Xh",
				conEntryPtr->upperReadQueue));
			qenable(WR(conEntryPtr->upperReadQueue));
		}
	}
	freemsg(mp);
	NWSLOG((SPXID, destConId, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: lrp: EXIT GotAck"));	
	NTR_VLEAVE();
	return;
}

/*
 * void SpxOrdRelReq(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
/*ARGSUSED*/
void
SpxOrdRelReq(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;
	register spxHdr_t *spxHdr, *inSpxHdr;
	struct T_ordrel_ind *tOrdInd;
	uint16	spxSeqNumber, spxAckNumber, spxAllocNumber, locAckNumber;
	mblk_t 	*newMp;
	uint16	destConId;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	spxHdr = (spxHdr_t *)mp->b_rptr;
	destConId = PGETINT16(&spxHdr->destinationConnectionId);
	destConId &= CONIDTOINDEX;
	conEntryPtr = &spxConnectionTable[destConId];

	NWSLOG((SPXID, destConId, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: lrp: ENTER OrdRelReq"));

	if(conEntryPtr->window.local.lastReNegReq_AckNum) {
		conEntryPtr->window.local.ack = 
				conEntryPtr->window.local.lastReNegReq_AckNum;
		conEntryPtr->window.local.lastReNegReq_AckNum = 0;
	}

	spxSeqNumber = PGETINT16(&spxHdr->sequenceNumber);
	spxAckNumber = PGETINT16(&spxHdr->acknowledgeNumber);
	spxAllocNumber = PGETINT16(&spxHdr->allocationNumber);
	locAckNumber = conEntryPtr->window.local.ack;

	/* This is not the expected Orderly Release, other endpoint must
	   of missed our Ack. Re-send ACK if we have changed states. */

	if ((spxSeqNumber < locAckNumber) && 
		((conEntryPtr->state == TS_WREQ_ORDREL) || 
		(conEntryPtr->state == TS_IDLE))) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: OrdRelReq repeated %d",spxSeqNumber));
			spxStats.spx_rcv_dup_packet++;
			freemsg(mp);
			SpxSendAck(destConId, FALSE, 0);
			goto Exit;
	}

	if (spxSeqNumber != locAckNumber) {		/* packet in sequence? */
		NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE, 
			"NSPX: lrp: OrdRelReq: orderly release Out Of Sequence."));
		freemsg(mp);
		goto Exit;
	}

	if ((conEntryPtr->state != TS_DATA_XFER ) &&
		(conEntryPtr->state != TS_WIND_ORDREL)) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: OrdRelReq: Bad destination state %d",
				conEntryPtr->state));
			freemsg(mp);
			spxStats.spx_rcv_bad_packet++;
			goto Exit;
	}

	/* is SPX acknowledge number greater than last ack number?
	   if so this is a piggyback-ack
	*/
	if(spxAckNumber > conEntryPtr->window.remote.ack) {
		NWSLOG((SPXID, destConId, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: lrp: OrdRelReq piggy-back Ack"));
		if ((newMp = copyb(mp)) != NULL) {
			inSpxHdr = (spxHdr_t *)newMp->b_rptr;
			/* zero out sequence Number this is a ACK not a NAK */
			inSpxHdr->sequenceNumber = 0;
			SpxAck(q,newMp);
		} else {
			spxStats.spx_alloc_failures++; 
			NWSLOG((SPXID,destConId, PNW_ERROR, SL_TRACE,
			"NSPX: lrp: OrdRelReq Data cant copy block "));
		}
	} else {
		/* packet is good, update remote allocation number if window
		 has been increased.
		*/
		if (conEntryPtr->window.remote.alloc <  spxAllocNumber)
			conEntryPtr->window.remote.alloc = spxAllocNumber;

		if (!conEntryPtr->disabled) {
			qenable(WR(conEntryPtr->upperReadQueue));
		}
	}
	locAckNumber++; 
	conEntryPtr->window.local.ack = locAckNumber;

/* send Acknowledge to endpoint  for OrlRel*/
	if(SpxSendAck(destConId, FALSE, 0)) {
		NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
			"NSPX: SpxOrdRelReq: send OrdRelAck Failed"));
		freemsg(mp);
		goto Exit;
	}

	if (conEntryPtr->state == TS_WIND_ORDREL)
		conEntryPtr->state = TS_IDLE;
	else 
		conEntryPtr->state = TS_WREQ_ORDREL;

/* build and send up t_ordrel_ind */
	MTYPE(mp) = M_PROTO;	
	mp->b_wptr = mp->b_rptr + sizeof(struct T_ordrel_ind);
	tOrdInd = (struct T_ordrel_ind *)mp->b_rptr;
	tOrdInd->PRIM_type = T_ORDREL_IND;
		
	if ((!canputnext(conEntryPtr->upperReadQueue)) ||
			 (conEntryPtr->flowControl == TRUE)) {
		NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE, 
			"NSPX: lrp: OrdRelReq: cant put up to q %Xh",
			conEntryPtr->upperReadQueue));
		putq(conEntryPtr->upperReadQueue,mp);
	} else {
		putnext(conEntryPtr->upperReadQueue,mp);
		conEntryPtr->window.local.alloc++;
	}

Exit:

	NWSLOG((SPXID, destConId, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: lrp: EXIT OrdRelReq"));
	NTR_VLEAVE();
	return;
}

/*
 * void  SpxData(queue_t *q, mblk_t *mp, int reentrance) 
 *	This is called when we receive a data packet we create a T_data_ind 
 *	and add the data on the end.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
/*ARGSUSED*/
void
SpxData(queue_t *q, mblk_t *mp, int reentrance) 
{
	register spxConnectionEntry_t *conEntryPtr;
	register spxHdr_t *inSpxHead;

	spxHdr_t *inSpxHdr;
	struct 	T_data_ind *tDataInd;
	mblk_t 	*dataMp, *newMp;
	queue_t	*upperQueue;
	uint16 	destConId;
	uint16 	spxDataSize, spxPacketSize;
	uint16 	spxSeqNumber, spxAckNumber, spxAllocNumber;
	uint 	msgSize;
	uint16 	ackNum;
	uint8	connectionControl;
	uint16 	locAckNumber;
	int		nextMsg, saveMsg;
	uint16 	i,tmp,numberMissed;
	int		sendAck;
	uint32	lspxSeqNumber,lspxAckNumber,lspxAllocNumber;
	uint32	llocAckNumber;
	uint32	llastAdvAlloc;


	NTR_ENTER( 3, q, mp, reentrance, 0, 0);
	inSpxHead = (spxHdr_t *)mp->b_rptr;
	destConId = PGETINT16(&inSpxHead->destinationConnectionId);
	destConId &= CONIDTOINDEX;

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"NSPX: lrp: Data: ENTER "));

	conEntryPtr = &spxConnectionTable[destConId];
	upperQueue = conEntryPtr->upperReadQueue;
	if(!reentrance)	/* first time, from spxlrput*/
		sendAck = 0;

	if ((conEntryPtr->state != TS_DATA_XFER ) &&
		(conEntryPtr->state != TS_WIND_ORDREL)) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
				"NSPX: lrp: Data: Bad destination state %d",
				conEntryPtr->state));
			freemsg(mp);
	 		spxStats.spx_rcv_bad_data_packet++; 
			conEntryPtr->conStats.con_rcv_bad_data_packet++; 
			goto Exit;
	}
	connectionControl = inSpxHead->connectionControl;
	spxPacketSize = PGETINT16(&inSpxHead->ipxHdr.len);

/* 
	if we got a larger than standard spx packet maybe the length
	was clobbered on the wire we will let the remote endpoint 
	resend the packet.  If the remote endpoint intentionally sent
	an oversize packet the connection will fail from his side and
	WatchEmu will have to kill this side.
*/
	if (spxPacketSize > conEntryPtr->maxSDU) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: lrp: Data packet data size %d too big Max=%d",
			(uint) spxPacketSize, conEntryPtr->maxSDU));
		spxStats.spx_rcv_bad_data_packet++; 
		conEntryPtr->conStats.con_rcv_bad_data_packet++; 
		freemsg(mp);
		goto Exit;
	}
	if(conEntryPtr->window.local.lastReNegReq_AckNum) {
		conEntryPtr->window.local.ack = 
				conEntryPtr->window.local.lastReNegReq_AckNum;
		conEntryPtr->window.local.lastReNegReq_AckNum = 0;
	}

	spxSeqNumber = PGETINT16(&inSpxHead->sequenceNumber);
	locAckNumber = conEntryPtr->window.local.ack;
		
	/* Adjust values need for testing if packet has already beeen received. */
		/* adjust locAckNumber for case:
		 *  locAck just wrapped, locAlloc has wrapped and received a
		 *  previous packet.
		 *	 locAck=1 locAlloc=8  sequence=0xfffe
		 */
	if ((spxSeqNumber > 0xff80) && (locAckNumber < 0x0080))
		llocAckNumber = (locAckNumber + 0x10000);
	else
		llocAckNumber = locAckNumber;

		/* adjust lspxSeqNumber for case:
		 *  locAck has not wrapped, locAlloc has wrapped and received a
		 *  packet which the seqNuber has wrapped..
		 *		locAck=0xfffc locAlloc=0x3  sequence= 0x00 thru 0x03
		 */
	if ((spxSeqNumber < 0x0080) && (locAckNumber > 0xff80))
		lspxSeqNumber = (spxSeqNumber + 0x10000);
	else
		lspxSeqNumber = spxSeqNumber;

	/*
	**	This is not the expected data buffer, it is a previous one.
	**	Acknowledge it again. 
	*/
	if (lspxSeqNumber < llocAckNumber) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: lrp: duplicated Data packet %d",spxSeqNumber));
		ackNum = PGETINT16(&inSpxHead->acknowledgeNumber);
		conEntryPtr->conStats.con_rcv_dup_packet++; 
	 	spxStats.spx_rcv_dup_packet++; 
		freemsg(mp);
		if (conEntryPtr->protocol & SPX_SPXII) {
			if (connectionControl & SPX_ACK_REQUESTED)
				SpxSendAck(destConId, FALSE, 0);
		} else
			SpxSendAck(destConId, TRUE, ackNum);
		goto Exit;
	}

	if (spxSeqNumber == locAckNumber) {		/* packet in sequence? */
		if (conEntryPtr->protocol & SPX_SPXII) {
			spxDataSize = (spxPacketSize - SPXII_PACKET_HEADER_SIZE);
			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX: lrp: SpxIIData: size %d seq=%d",
				(uint)spxDataSize, spxSeqNumber));

			/* Adjust values need for testing if packet is a piggy-back ACK. */

			spxAckNumber = PGETINT16(&inSpxHead->acknowledgeNumber);

			/* adjust lspxAckNumber for case:
			 *  remote.ack(last acknowledge # from other endpoint) 0xfffd
			 *  spxAckNumber(acknowledge # in this packet) 0x002
			 */
			if ((conEntryPtr->window.remote.ack > 0xff80) && 
													(spxAckNumber < 0x0080))
				lspxAckNumber = (spxAckNumber + 0x10000);
			else
				lspxAckNumber = spxAckNumber;

			/* is SPX acknowledge number greater than last ack number?
			 * if so this is a piggyback-ack
			 */
			if(lspxAckNumber > (uint32)conEntryPtr->window.remote.ack) {
				NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
					"NSPX: lrp: SpxData piggy-back Ack"));

				if ((newMp = copyb(mp)) != NULL) {
					inSpxHdr = (spxHdr_t *)newMp->b_rptr;
					/* zero out sequence Number this is a ACK not a NAK */
					inSpxHdr->sequenceNumber = 0;
					SpxAck(q,newMp);
				} else {
					/* can't copyb  forget about the piggy-back Ack */
					spxStats.spx_alloc_failures++; 
					NWSLOG((SPXID,0, PNW_ERROR, SL_TRACE,
					"NSPX: lrp: Data cant copy block "));
				}
			} else {
				/* Adjust values need for testing if packet increasing the 
				 * remote.alloc number.  Other endpoint is increasing his 
				 * current window size.
				 */

				spxAllocNumber = PGETINT16(&inSpxHead->allocationNumber);

				/* adjust lspxAllocNumber for case:
				 *  remote.alloc(last Allocation # from other endpoint) 0xfffd
				 *  spxAllocNumber(Allocation # in this packet) 0x002
				 */
				if ((conEntryPtr->window.remote.alloc > 0xff80) && 
											(spxAllocNumber < 0x0080))
					lspxAllocNumber = (spxAllocNumber + 0x10000);
				else
					lspxAllocNumber = spxAllocNumber;

				/* packet is good, update remote allocation number if window
				 has been increased.
				*/
				if (lspxAllocNumber > conEntryPtr->window.remote.alloc)
					conEntryPtr->window.remote.alloc = spxAllocNumber;

				if (!conEntryPtr->disabled) {
					qenable(WR(conEntryPtr->upperReadQueue));
				}

			}
			if (conEntryPtr->specialFlags & SPX_SAVE_SEND_HEADER_FLAG) {
				NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE, 
					"NSPX: lrp: leaving Header info on data"));
				spxDataSize += sizeof(spxIIHdr_t);
			} else {
				mp->b_rptr += SPXII_PACKET_HEADER_SIZE;
			}
		} else { /* Not SPXII data */
			spxDataSize = (spxPacketSize - SPX_PACKET_HEADER_SIZE);
			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX: lrp: SpxData: size %d seq=%d",
				(uint)spxDataSize, spxSeqNumber));
			if (conEntryPtr->specialFlags & SPX_SAVE_SEND_HEADER_FLAG) {
				NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE, 
					"NSPX: lrp: leaving Header info on data"));
				spxDataSize += sizeof(spxHdr_t);
			} else {
				mp->b_rptr += SPX_PACKET_HEADER_SIZE;
			}
		}

		msgSize = msgdsize(mp);
		if (msgSize != spxDataSize ) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: SpxData: Data size %d does not match size in packet %d",
					msgSize, (uint)spxDataSize));
			spxStats.spx_rcv_bad_data_packet++; 
			conEntryPtr->conStats.con_rcv_bad_data_packet++; 
			freemsg(mp);
			NTR_VLEAVE();
			return;
		}

		/*
		if we cant alloc a header let the remote endpoint resend this
		packet and maybe then we can allocate a buffer.
		*/
		if ((dataMp=allocb(sizeof(struct T_data_ind), BPRI_MED)) == NULL) {
			spxStats.spx_alloc_failures++; 
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
				"NSPX: lrp: Data cant alloc %d",
				sizeof(struct T_data_ind)));
			freemsg(mp);
			goto Exit;
		}

		tDataInd = (struct T_data_ind *)dataMp->b_rptr;
		tDataInd->PRIM_type = T_DATA_IND;
		if (connectionControl & (uint8) SPX_EOF_BIT)
			tDataInd->MORE_flag = FALSE;
		else 
			tDataInd->MORE_flag = TRUE;
		MTYPE(dataMp) = M_PROTO;
		dataMp->b_wptr = dataMp->b_rptr + sizeof(struct T_data_ind);

		dataMp->b_cont = mp;
	
		if ((!canputnext(upperQueue)) || (conEntryPtr->flowControl == TRUE)) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
				"NSPX: lrp: Data cant putup seq %d to q %Xh",
				(uint)spxSeqNumber, upperQueue));
			conEntryPtr->conStats.con_rcv_packet_qued++; 
			conEntryPtr->flowControl = TRUE;
			putq(upperQueue,dataMp);
		} else {
			spxStats.spx_rcv_packet_sentup++; 
			conEntryPtr->conStats.con_rcv_packet_sentup++; 
			putnext(upperQueue,dataMp);
			conEntryPtr->window.local.alloc++;
		}

		/* Update counters */
		locAckNumber++; 
		conEntryPtr->window.local.ack = locAckNumber;
		if (connectionControl & SPX_ACK_REQUESTED)
			sendAck++;		/* send Acknowledge after all packets processed*/

		/*  see if next packet is in out of sequence area.
			if so process it now
		*/
		nextMsg = ((uint16)(spxSeqNumber + 1) % conEntryPtr->spxWinSize);
		if (conEntryPtr->window.outOfSeqPackets[nextMsg] != NULL) {
			mp = conEntryPtr->window.outOfSeqPackets[nextMsg];
			conEntryPtr->window.outOfSeqPackets[nextMsg] = NULL;
			SpxData(q,mp,TRUE);
		}
		if (sendAck && !reentrance)
			SpxSendAck(destConId, FALSE, 0);

	} else { /* end packet in sequence */

		/* Adjust values need for testing if packet is a future packet but 
		 * within current receive window.
		 */

		/* adjust llastAdvAlloc for case:
		 *  lastAdvertisedAlloc just wrapped, and received a packet with a sequence
		 *  number that has not wrapped.
		 *		lastAdvAlloc=4  sequence=0xfffe
		 */
		if ((spxSeqNumber >= 0xff80) &&
						(conEntryPtr->window.local.lastAdvertisedAlloc < 0x0080 ))
			llastAdvAlloc = 
					conEntryPtr->window.local.lastAdvertisedAlloc + 0x10000;
		else
			llastAdvAlloc = conEntryPtr->window.local.lastAdvertisedAlloc;


		if ((conEntryPtr->protocol & SPX_SPXII ) && 
					((uint32)spxSeqNumber <= llastAdvAlloc)) {
	 		/* SPXII packet out of sequence, but within the allocated window.
			   save the packet to process later
			*/
			NWSLOG((SPXID, 0, PNW_ERROR, SL_ERROR, 
				"NSPX: lrp: future SPXII packet %d received",spxSeqNumber));
			conEntryPtr->conStats.con_rcv_packet_outseq++; 

			saveMsg = spxSeqNumber % conEntryPtr->spxWinSize;
			if (conEntryPtr->window.outOfSeqPackets[saveMsg] != NULL) {
				freemsg(conEntryPtr->window.outOfSeqPackets[saveMsg]);
			}
			conEntryPtr->window.outOfSeqPackets[saveMsg] = mp;

			/* if ACK requested on out of sequence packet, must of missed 
			   some messages. send NAK  */
			if (connectionControl & SPX_ACK_REQUESTED) {
				/* search backwards from current out of sequence packet for the
				   first NULL.  This will be the last packet missed. the difference
				   the last packet missed and the last in sequence is the number of
				   packets missed.
				*/
				for(i=0; i < conEntryPtr->spxWinSize; i++) {
					tmp = (uint16)(spxSeqNumber - i) % conEntryPtr->spxWinSize;
					if (conEntryPtr->window.outOfSeqPackets[tmp] == NULL)
						break;
				}

		  		numberMissed = ((spxSeqNumber -i) - (locAckNumber - 1));
				SpxSendNak(destConId, numberMissed);
			}
		} else {
			/*
			this is a out of sequence packet on a SPX connection, free the
			message and continue.
			*/
			NWSLOG((SPXID, 0, PNW_ERROR, SL_ERROR, 
				"NSPX: lrp: future packet %d recieved dicard it",spxSeqNumber));
	 		spxStats.spx_rcv_bad_data_packet++; 
			conEntryPtr->conStats.con_rcv_bad_data_packet++; 
			freemsg(mp);
		}
	}
	Exit:

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: lrp: EXIT Data"));
	NTR_VLEAVE();
	return;
}

/*
 * void SpxGetOptions(spxConnectionEntry_t *conEntryPtr, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxGetOptions(spxConnectionEntry_t *conEntryPtr, mblk_t *mp)
{
	register negoHdr_t *negotPacket;
	Opt32_t *optPacket;
	Opt8_t *opt8Packet;
	uint32 usecToNet;
	uint16 optionCount;
	uint8  optionType,optionSize,optType;

	NTR_ENTER( 2, conEntryPtr, mp, 0, 0, 0);
	if (conEntryPtr == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: SpxGetOptions called with NULL ptr")); 
		NTR_VLEAVE();
		return;
	}
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: ENTER GetOptions")); 

	if (msgdsize(mp) > (SPXII_PACKET_HEADER_SIZE + sizeof(uint32))) {
		if (mp->b_cont) {
			if ( pullupmsg(mp,-1) == 0 ) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				 	"NSPX: GetOptions cant pullupmsg "));
				goto Exit;
			}
		}
		negotPacket = (negoHdr_t *)(mp->b_rptr + SPXII_PACKET_HEADER_SIZE);
		optionCount = PGETINT16(&negotPacket->numberOfOptions);
		/* adjust pointer to first option */
		mp->b_rptr += (SPXII_PACKET_HEADER_SIZE + sizeof(uint16));
		while ( optionCount ) {
			optionCount--;
			optionType = *mp->b_rptr;
			switch (optionType) {
				case SPXII_RTT :
					NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
						"NSPX: GetOptions: SWITCH option type: SPXII_RTT"));
					optPacket = (Opt32_t *)(mp->b_rptr);
					usecToNet = PGETINT32(&optPacket->Value);
					conEntryPtr->ticksToNet = 1+((HZ * usecToNet)/ 1000000);
					mp->b_rptr += sizeof(Opt32_t); /* pointer to next option */
					break;
				case IPX_CHKSM :
					opt8Packet = (Opt8_t *)(mp->b_rptr);
					NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
						"NSPX: GetOptions: SWITCH option type: IPX_CHKSM =%x",
						opt8Packet->Value));
					if ((opt8Packet->Value) && (conEntryPtr->ipxChecksum)) {
						NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
						  "NSPX:GetOptions:Set Checksums ON"));
						conEntryPtr->ipxChecksum = TRUE;
					} else {
						NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
						  "NSPX:GetOptions:Set Checksums OFF"));
						conEntryPtr->ipxChecksum = FALSE;
					}
					mp->b_rptr += sizeof(Opt8_t); /* pointer to next option */
					break;
				default : /* unknown option adjust to next option */
					NWSLOG((SPXID, 0, PNW_SWITCH_DEFAULT, SL_TRACE,
					   "NSPX: GetOptions: OptionType DEFAULT 0x%x",optionType));
					if ((optionType & EXT_ID_MASK) == EXT_ID_MASK)	
						optType = optionType;
					else
						optType = (optionType & OPT_SIZE_MASK);
					switch (optType) {
						case U8 :
							NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
								"NSPX: GetOpts: SWITCH default size: U8"));
							mp->b_rptr += sizeof(Opt8_t); /* next option */
							break;
						case U16 :
							NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
								"NSPX: GetOpts: SWITCH default size: U16"));
							mp->b_rptr += sizeof(Opt16_t); /* next option */
							break;
						case U32 :
							NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
								"NSPX: GetOpts: SWITCH default size: U32"));
							mp->b_rptr += sizeof(Opt32_t); /* next option */
							break;
						case UEXT :
							optionSize = (*(mp->b_rptr + sizeof(uint8)));
							NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
								"NSPX: GetOpts: SWITCH default size: UEXT=%d",
								optionSize));
							/* next option */
							mp->b_rptr += (optionSize + sizeof(uint16));
							break;
						case EID_U8 : /* Extended ID UByte */
							NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
								"NSPX: GetOpts: SWITCH default size:EIDU8"));
							mp->b_rptr += sizeof(uint8); /* skip Extend ID */
							mp->b_rptr += sizeof(Opt8_t); /* next option */
							break;
						case EID_U16 : /* Extended ID UInt16 */
							NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
								"NSPX: GetOpts: SWITCH default size:EIDU16"));
							mp->b_rptr += sizeof(uint8); /* skip Extend ID */
							mp->b_rptr += sizeof(Opt16_t); /* next option */
							break;
						case EID_U32 : /* Extended ID UInt32 */
							NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
								"NSPX: GetOpts: SWITCH default size:EIDU32"));
							mp->b_rptr += sizeof(uint8); /* skip Extend ID */
							mp->b_rptr += sizeof(Opt32_t); /* next option */
							break;
						case EID_UEXT :
							mp->b_rptr += sizeof(uint8); /* skip Extend ID */
							optionSize = (*(mp->b_rptr + sizeof(uint8)));
							NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
								"NSPX: GetOpts: default size: EIDUExt=%d",
								optionSize));
							/* next option */
							mp->b_rptr += (optionSize + sizeof(uint16));
							break;
					}
					break;
			}
		}
	}
	Exit:
		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: SpxGetOptions EXIT"));
		NTR_VLEAVE();
		return;
}

/*
 * void SpxNegotiateReq(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxNegotiateReq(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;
	register spxIIHdr_t *spxIIHeader;
	uint	destConId;
	uint16	negotSize,i;
	mblk_t	*spxAckMp, *tmp;
	uint32	llocAck, llocAlloc, llocSeq;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	spxIIHeader = (spxIIHdr_t *)mp->b_rptr;
	destConId = PGETINT16(&spxIIHeader->destinationConnectionId);
	destConId &= CONIDTOINDEX;
	conEntryPtr = &spxConnectionTable[destConId];

	NWSLOG((SPXID, destConId, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: lrp: NegotiateReq ENTER q= %X conId= 0x%X",q, destConId));

	if (conEntryPtr->sessionFlags & WAIT_NEGOTIATE_REQ) {
		conEntryPtr->sessionFlags &= ~WAIT_NEGOTIATE_REQ;
		/* cancel negotiation TIMEOUT here */
		if (conEntryPtr->nTimeOutId) {
			untimeout(conEntryPtr->nTimeOutId);
			conEntryPtr->nTimeOutId = 0;
		}
		if (msgdsize(mp) > SPXII_PACKET_HEADER_SIZE) {
			negotSize = PGETINT16(&spxIIHeader->negotiateSize);
			SpxGetOptions(conEntryPtr, mp);
			if (negotSize < (uint16) SPX_DEFAULT_PACKET_SIZE)
				negotSize = SPX_DEFAULT_PACKET_SIZE;
		} else {
			negotSize = SPX_DEFAULT_PACKET_SIZE;
		}
		if (conEntryPtr->maxSDU < SPX_DEFAULT_PACKET_SIZE)
			conEntryPtr->maxSDU = SPX_DEFAULT_PACKET_SIZE;

		conEntryPtr->maxTPacketSize = 
		conEntryPtr->maxSDU = min(conEntryPtr->maxSDU,negotSize);

		negotSize = PGETINT16(&spxIIHeader->ipxHdr.len);
		if (negotSize < (uint16) SPX_DEFAULT_PACKET_SIZE)
			negotSize = SPX_DEFAULT_PACKET_SIZE;

		conEntryPtr->maxRPacketSize = min(conEntryPtr->maxSDU,negotSize);
	
		SpxSendAck(destConId, FALSE, 0);
		conEntryPtr->sessionFlags |= SESS_SETUP;
		conEntryPtr->sizeNegoRetry = 0;
		SpxSendNegSessPkt(conEntryPtr);
	} else {
	/* this must be re-negotiation, this might be only on Recieve route. Get
	   new packet size and send an Negotiation Ack back */

		NWSLOG((SPXID, destConId, PNW_ALL, SL_TRACE,
			"NSPX: NegotiateReq : received Re-negotiation Request"));

		/* if seq # is not zero this is a re_nego REQ,
		   if seq # is zero this is only a delayed Neg/Sess Req in this
			case just send the ACK back do not reduce size. */ 
		if(spxIIHeader->sequenceNumber != 0) {
			if (msgdsize(mp) > SPXII_PACKET_HEADER_SIZE) {
				negotSize = PGETINT16(&spxIIHeader->ipxHdr.len);
				if (negotSize < (uint16) SPX_DEFAULT_PACKET_SIZE)
					negotSize = SPX_DEFAULT_PACKET_SIZE;
			} else {
				negotSize = SPX_DEFAULT_PACKET_SIZE;
			}
			conEntryPtr->maxRPacketSize = negotSize;
			NWSLOG((SPXID, destConId, PNW_ALL, SL_TRACE,
				"NSPX: NegotiateReq : Set maxRPacketSize to %d",negotSize));

			/* Free all OutofSequence Packets, they will be resent */
			for (i = 0; i < MAX_SPX2_WINDOW_SIZE; i++) {
				if ((tmp = conEntryPtr->window.outOfSeqPackets[i]) != NULL) {
					freemsg(tmp);
					conEntryPtr->window.outOfSeqPackets[i] = (mblk_t *)NULL;
				}
			} 
			/* Adjust Alloc Number. Since there might be a hole in the
			 * next sequence Numbers received (sender flush unAcked msgs
			 * and does not reuse the those sequence numbers) save the 
			 * sequence Number of the ReNeg Req it will be the seq number
			 * of the next data packet. We will uss this as our local ack
			 * number when we receive the next data packet.  */

			/*
			 * We have to be a little bit careful how we do the arithmetic 
			 * here.  The values could wrap around so we might not get what 
			 * we expect.
			 */
			if ((conEntryPtr->window.local.alloc < 0x0080) &&
				(conEntryPtr->window.local.ack > 0xff80))  {
				llocAlloc = conEntryPtr->window.local.alloc + 0x10000;
			} else {
				llocAlloc = conEntryPtr->window.local.alloc;
			}

			llocAck = conEntryPtr->window.local.ack;
			llocSeq = GETINT16(spxIIHeader->sequenceNumber);

			conEntryPtr->window.local.alloc = 
					(uint16) ((llocAlloc - llocAck + llocSeq) & 0xFFFF);
			conEntryPtr->window.local.lastReNegReq_AckNum = 
				GETINT16(spxIIHeader->sequenceNumber);

		}

		if (spxbot == NULL) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: NegotiateReq : spxbot is NULL"));
			goto Exit;
		}   

		if (!canputnext(spxbot)) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: NegotiateReq : cannot put down to q %Xh",spxbot));
			conEntryPtr->conStats.con_canput_fail++; 
			goto Exit;
		}   

		conEntryPtr->spxHdr.connectionControl = conEntryPtr->protocol;
 
		if ( (spxAckMp = SpxSetUpPacket(destConId)) == NULL ) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: NegotiateReq : SetUpPacket failed"));
			goto Exit;
		}
 
		spxIIHeader = (spxIIHdr_t *)spxAckMp->b_rptr;
		spxIIHeader->connectionControl |= (SPX_SYSTEM_PACKET | SPX_NEG);
		spxIIHeader->ipxHdr.len = GETINT16( msgdsize(spxAckMp) );
		spxIIHeader->sequenceNumber = (uint16)0;
		conEntryPtr->conStats.con_send_ack++; 
		putnext(spxbot,spxAckMp);
	}
	Exit:
		freemsg(mp);
		NTR_VLEAVE();
		return;
}

/*
 * mblk_t * SpxSetUpConnCon(spxConnectionEntry_t *conEntryPtr)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
mblk_t *
SpxSetUpConnCon(spxConnectionEntry_t *conEntryPtr)
{
	mblk_t *mp;
	uint conId;
	struct T_conn_con *tConnCon;
	uint	tConnConSize;
	SPX2_OPTIONS *spx2OptRet;


	NTR_ENTER( 1, conEntryPtr, 0, 0, 0, 0);
	if (conEntryPtr == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: SpxSetUpConnCon called with NULL ptr")); 
		return( (mblk_t *)NTR_LEAVE(NULL));
	}
	conId = conEntryPtr->spxHdr.sourceConnectionId;
	conId &= CONIDTOINDEX;
	NWSLOG((SPXID, conId, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: ENTER SetUpConnCon")); 

	if (conEntryPtr->spx2Options)
		tConnConSize = (sizeof(struct T_conn_con) + sizeof(ipxAddr_t) 
			+ sizeof(SPX2_OPTIONS));
	else
		tConnConSize = (sizeof(struct T_conn_con) + sizeof(ipxAddr_t) 
			+ sizeof(SPX_OPTS));
	if ((mp = allocb(tConnConSize, BPRI_MED)) == NULL) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, conId, PNW_ERROR, SL_TRACE,
			"NSPX: SpxSetUpConnCon: couldnt alloc msg size %d",
			tConnConSize));
		return( (mblk_t *)NTR_LEAVE(NULL));
	}
	
	tConnCon = (struct T_conn_con *)mp->b_rptr;
	mp->b_wptr = (mp->b_rptr + tConnConSize);
	MTYPE(mp) = M_PROTO;
	tConnCon->PRIM_type = T_CONN_CON;
	tConnCon->RES_length = sizeof(ipxAddr_t);
	tConnCon->RES_offset = sizeof(struct T_conn_con);
	if (conEntryPtr->spx2Options)
		tConnCon->OPT_length = sizeof(SPX2_OPTIONS);
	else
		tConnCon->OPT_length = sizeof(SPX_OPTS); 

	tConnCon->OPT_offset = (sizeof(struct T_conn_con)+sizeof(ipxAddr_t));
	
	if (conEntryPtr->spx2Options) {
		bcopy((char *)&conEntryPtr->spxHdr.ipxHdr.dest,
			(char *)(mp->b_rptr +sizeof(struct T_conn_con)),
			IPX_ADDR_SIZE);
		spx2OptRet = (SPX2_OPTIONS *)
		(mp->b_rptr + sizeof(struct T_conn_con) + sizeof(ipxAddr_t));

		spx2OptRet->versionNumber = OPTIONS_VERSION;
		if (conEntryPtr->sessionFlags & NO_NEGOTIATE)
			spx2OptRet->spxIIOptionNegotiate = SPX2_NO_NEGOTIATE_OPTIONS;
		else
			spx2OptRet->spxIIOptionNegotiate = SPX2_NEGOTIATE_OPTIONS;
		spx2OptRet->spxIIRetryCount = conEntryPtr->tMaxRetries;
		spx2OptRet->spxIIMinimumRetryDelay = 
			TCKS2MSEC(conEntryPtr->minTicksToWait);
		spx2OptRet->spxIIMaximumRetryDelta = 
			TCKS2MSEC(conEntryPtr->maxTicksToWait);
		spx2OptRet->spxIILocalWindowSize = conEntryPtr->spxWinSize;
		spx2OptRet->spxIIWatchdogTimeout = TCKS2MSEC(spxWatchEmuInterval);
		spx2OptRet->spxIIConnectionTimeout = 0;
		spx2OptRet->spxIIRemoteWindowSize = 
			conEntryPtr->window.remote.alloc;
		spx2OptRet->spxIIConnectionID = 
			GETINT16(conEntryPtr->spxHdr.destinationConnectionId);
		spx2OptRet->spxIIInboundPacketSize = conEntryPtr->maxRPacketSize;
		spx2OptRet->spxIIOutboundPacketSize = conEntryPtr->maxTPacketSize;
		spx2OptRet->spxIISessionFlags = SPX_SF_NONE;
		if (conEntryPtr->ipxChecksum)
			spx2OptRet->spxIISessionFlags |= SPX_SF_IPX_CHECKSUM;
		if (conEntryPtr->protocol & SPX_SPXII)
			spx2OptRet->spxIISessionFlags |= SPX_SF_SPX2_SESSION;
	} else {
		bcopy((char *)&conEntryPtr->spxHdr.ipxHdr.dest,
			(char *)(mp->b_rptr +sizeof(struct T_conn_con)), 
			IPX_ADDR_SIZE);

		GETALIGN16(&conEntryPtr->spxHdr.destinationConnectionId,
			(mp->b_rptr + sizeof(struct T_conn_con) + sizeof(ipxAddr_t)));
		GETALIGN16(&conEntryPtr->window.remote.alloc,
			(mp->b_rptr + sizeof(struct T_conn_con) + sizeof(ipxAddr_t) +
			sizeof(uint16)));
	}
	return((mblk_t *)NTR_LEAVE((int)mp));
}

/*
 * void SpxSessionSetUp(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxSessionSetUp(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;
	register spxIIHdr_t *spxIIHeader;
	uint16 negotSize;
	uint destConId;
	queue_t *upperQueue;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	spxIIHeader = (spxIIHdr_t *)mp->b_rptr;
	destConId = PGETINT16(&spxIIHeader->destinationConnectionId);
	destConId &= CONIDTOINDEX;
	conEntryPtr = &spxConnectionTable[destConId];

	NWSLOG((SPXID, destConId, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: lrp: SessionSetup ENTER q= %X conId= 0x%X",q, destConId));

	if (conEntryPtr->sessionFlags & WAIT_NEGOTIATE_REQ ) {
		conEntryPtr->sessionFlags &= ~WAIT_NEGOTIATE_REQ;
		/* cancel negotiation TIMEOUT here */
		if (conEntryPtr->nTimeOutId) {
			untimeout(conEntryPtr->nTimeOutId);
			conEntryPtr->nTimeOutId = 0;
		}
		conEntryPtr->spxHdr.ipxHdr.dest = spxIIHeader->ipxHdr.src;
		conEntryPtr->spxHdr.destinationConnectionId =
								spxIIHeader->sourceConnectionId;
		conEntryPtr->spxHdr.sequenceNumber =0;
		conEntryPtr->spxHdr.acknowledgeNumber = 0;
		conEntryPtr->spxHdr.allocationNumber = 0;

		if (msgdsize(mp) > SPXII_PACKET_HEADER_SIZE) {
			SpxGetOptions(conEntryPtr, mp);
			negotSize = PGETINT16(&spxIIHeader->ipxHdr.len);
			if (negotSize < (uint16) SPX_DEFAULT_PACKET_SIZE)
				negotSize = SPX_DEFAULT_PACKET_SIZE;
		} else
			negotSize = SPX_DEFAULT_PACKET_SIZE;

		if (conEntryPtr->maxSDU < SPX_DEFAULT_PACKET_SIZE)
			conEntryPtr->maxSDU = SPX_DEFAULT_PACKET_SIZE;

		conEntryPtr->maxRPacketSize = min(conEntryPtr->maxSDU,negotSize);

		conEntryPtr->window.remote.alloc = 
					PGETINT16(&spxIIHeader->allocationNumber);

		SpxSendAck(destConId, FALSE, 0);
		conEntryPtr->protocol &= ~SPX_NEG;	/*Clear Neg Bit Negotiation Done*/

		freemsg(mp);
		if ((mp = SpxSetUpConnCon(conEntryPtr)) == NULL) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: ConnReqAck: SpxSetupConnCon Failed "));
			goto Error;
		}

		upperQueue = conEntryPtr->upperReadQueue;

/*
	if we cant put up dont change the state so the remote endpoint
	can retry and maybe then we can establish a connection 
*/
		if (!canputnext(upperQueue)) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: SessionSetUp: cant put up to q %Xh",
				upperQueue));
			freemsg(mp);
			goto Error;
		}

		conEntryPtr->state = TS_DATA_XFER;
		if (conEntryPtr->ipxChecksum == TRUE)
			conEntryPtr->spxHdr.ipxHdr.chksum = IPX_CHKSUM_TRIGGER;
		
		putnext(upperQueue,mp);
	} else {
	/* Endpoint missed SPX Session SetUP ACK, we have already notified the
	   the user and changed states, so just resend the Ack.  Set SPX_NEG
	   temporarily so SpxSendAck will fill in the negotiation size.
	 */
		freemsg(mp);
		conEntryPtr->protocol |= SPX_NEG;
		SpxSendAck(destConId, FALSE, 0);
		conEntryPtr->protocol &= ~SPX_NEG;	/* Clear Size Bit */
	}
	NWSLOG((SPXID, destConId, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: lrp: EXIT SessionSetUp "));
	NTR_VLEAVE();
	return;


	Error:
		NWSLOG((SPXID, spxMinorDev, PNW_EXIT_ROUTINE, SL_TRACE,
					"NSPX: lrp: SessionSetUp EXIT error "));
		NTR_VLEAVE();
		return;
}

/*
 * void SpxConnReqAck(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
/*ARGSUSED*/
void
SpxConnReqAck(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;
	register spxHdr_t *spxHeader;
	register spxIIHdr_t *spxIIHeader;
	uint destConId;
	queue_t *upperQueue;
	uint16 negotSize;
	int	lastMsg;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	spxHeader = (spxHdr_t *)mp->b_rptr;
	destConId = PGETINT16(&spxHeader->destinationConnectionId);
	destConId &= CONIDTOINDEX;
	conEntryPtr = &spxConnectionTable[destConId];

	NWSLOG((SPXID, destConId, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: lrp: ConnReqAck ENTER "));

	lastMsg = 0;	/* Connect Request Always have Sequence Number 0 */
	if (conEntryPtr->tTimeOutId) {
		untimeout(conEntryPtr->tTimeOutId);
		conEntryPtr->tTimeOutId = 0;
		if(conEntryPtr->window.unAckedMsgs[lastMsg] != NULL) {
			freemsg(conEntryPtr->window.unAckedMsgs[lastMsg]);
		}
		conEntryPtr->window.unAckedMsgs[lastMsg] = NULL;
		conEntryPtr->needAck = FALSE;
		conEntryPtr->tRetries=0;
	}
	conEntryPtr->window.local.sequence = 0;

	if (conEntryPtr->state != TS_WCON_CREQ) {
		NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
			"NSPX: lrp: ConnReqAck: dest endpoint bad state %d",
			conEntryPtr->state));
		freemsg(mp);
		goto Error;
	}

	conEntryPtr->protocol &= spxHeader->connectionControl;
	conEntryPtr->spxHdr.ipxHdr.dest = spxHeader->ipxHdr.src;
	conEntryPtr->spxHdr.destinationConnectionId =
									spxHeader->sourceConnectionId;
	conEntryPtr->spxHdr.sequenceNumber =0;
	conEntryPtr->spxHdr.acknowledgeNumber = 0;
	conEntryPtr->spxHdr.allocationNumber = 0;

	if (conEntryPtr->protocol & SPX_SPXII ) {
		spxIIHeader = (spxIIHdr_t *)mp->b_rptr;
		conEntryPtr->sessionFlags |= WAIT_NEGOTIATE_REQ; 
		/* setup negotiation TIMEOUT, if no Session Setup within time 
		   period disconnect
		*/
		conEntryPtr->nTimeOutId = itimeout(SpxNegTimeOut,
			(void *)destConId, spxWatchEmuInterval, pltimeout);
		if (conEntryPtr->protocol & SPX_NEG ) {
			if (msgdsize(mp) > SPX_PACKET_HEADER_SIZE) {
				negotSize = PGETINT16(&spxIIHeader->negotiateSize);
				if (negotSize < (uint16)SPX_DEFAULT_PACKET_SIZE)
					negotSize = SPX_DEFAULT_PACKET_SIZE;
			} else
				negotSize = SPX_DEFAULT_PACKET_SIZE;

			if (conEntryPtr->maxSDU < SPX_DEFAULT_PACKET_SIZE)
				conEntryPtr->maxSDU = SPX_DEFAULT_PACKET_SIZE;

			conEntryPtr->maxSDU = min(conEntryPtr->maxSDU,negotSize);
		} else {
			conEntryPtr->maxSDU = SPX_DEFAULT_PACKET_SIZE;
			conEntryPtr->sessionFlags |= NO_NEGOTIATE;
		}
		conEntryPtr->maxTPacketSize = 
		conEntryPtr->maxRPacketSize = conEntryPtr->maxSDU;
		if (conEntryPtr->beginTicks <= conEntryPtr->endTicks)
			conEntryPtr->ticksToNet = 
				conEntryPtr->endTicks - conEntryPtr->beginTicks;
		else
			/* rollover ?*/
			conEntryPtr->ticksToNet = (uint32) 0xFFFFFFFF -
					(conEntryPtr->beginTicks - conEntryPtr->endTicks);
		conEntryPtr->sizeNegoRetry = 0;
		if (conEntryPtr->protocol & SPX_NEG )
			SpxSendNegSessPkt(conEntryPtr);
		freemsg(mp);
		NWSLOG((SPXID, destConId, PNW_EXIT_ROUTINE, SL_TRACE,
					"NSPX: lrp: EXIT ConnReqAck "));
		NTR_VLEAVE();
		return;
	} else {
		conEntryPtr->maxSDU =
		conEntryPtr->maxTPacketSize = 
		conEntryPtr->maxRPacketSize = (uint32)SPX_DEFAULT_PACKET_SIZE;
	
		/* we should only save the allocation number because this is the 
		   window size */
		conEntryPtr->window.remote.alloc = 
				PGETINT16(&spxHeader->allocationNumber);
		conEntryPtr->window.local.alloc = 0;

		freemsg(mp);
		if ((mp = SpxSetUpConnCon(conEntryPtr)) == NULL) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: ConnReqAck: SpxSetupConnCon Failed "));
			goto Error;
		}

		upperQueue = conEntryPtr->upperReadQueue;
		/*
		**  if we cant put up dont change the state so the remote endpoint
		**  can retry and maybe then we can establish a connection 
		*/
		if (!canputnext(upperQueue)) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: ConnReqAck: cant put up to q %Xh",
				upperQueue));
			freemsg(mp);
			goto Error;
		}

		conEntryPtr->state = TS_DATA_XFER;
		
		putnext(upperQueue,mp);

		NWSLOG((SPXID, destConId, PNW_EXIT_ROUTINE, SL_TRACE,
				"NSPX: lrp: EXIT ConnReqAck "));
		NTR_VLEAVE();
		return;
	}
	Error:
		NWSLOG((SPXID, spxMinorDev, PNW_EXIT_ROUTINE, SL_TRACE,
					"NSPX: lrp: ConnReqAck EXIT error "));
		NTR_VLEAVE();
		return;
}

/*
 * void SpxDisConnAck(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
/*ARGSUSED*/
void 
SpxDisConnAck(queue_t *q, mblk_t *mp)
{ 	
	register spxConnectionEntry_t *conEntryPtr;
	register spxHdr_t *spxHeader;
	uint destConId;
	mblk_t *flushMp;
	struct T_ok_ack *tOkAck;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	spxHeader = (spxHdr_t *)mp->b_rptr;
	destConId = PGETINT16(&spxHeader->destinationConnectionId);
	destConId &= CONIDTOINDEX;
	conEntryPtr = &spxConnectionTable[destConId];

	NWSLOG((SPXID, destConId, PNW_ENTER_ROUTINE, SL_TRACE,
			"NSPX: lrp: ENTER DisConAck"));

	conEntryPtr->sessionFlags &= ~WAIT_TERMINATE_ACK;

	if ((conEntryPtr->state == TS_IDLE) ||
		(conEntryPtr->state == TS_UNBND)) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
				"NSPX: DisConnAck: Bad destination state %d EXIT",
				conEntryPtr->state));
			freemsg(mp);
			NTR_VLEAVE();
			return;
	}

/* this connection is no more so the packet header is zerod
	we need to preserve our source socket and conn Id.
	everything in the spx header is zeroed and a few fields
	are reset.  The connection Entryt table is not so the optmgmt
	is still effective
*/
 
	SpxResetConnection(conEntryPtr);
 
	conEntryPtr->needAck = FALSE;
	conEntryPtr->state = TS_IDLE;
 
/* per tli spec we must flush before putting up */
	if ((flushMp = allocb(1, BPRI_MED)) == NULL ) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TDisReq cant alloc msg size 1"));
	} else {
		*(flushMp->b_rptr) = FLUSHR | FLUSHW; /* flushFlags */
		MTYPE(flushMp) = M_FLUSH;
		flushMp->b_wptr = flushMp->b_rptr + 1;
		putnext(conEntryPtr->upperReadQueue,flushMp);

		tOkAck = (struct T_ok_ack *)mp->b_rptr;
		tOkAck->PRIM_type = T_OK_ACK;
		tOkAck->CORRECT_prim = T_DISCON_REQ;
		mp->b_wptr = mp->b_rptr + sizeof(struct T_ok_ack);
		MTYPE(mp) = M_PCPROTO;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = 0;
		}
		putnext(conEntryPtr->upperReadQueue,mp);
	}

	NWSLOG((SPXID, destConId, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: lrp: EXIT DisConAck"));
	NTR_VLEAVE();
	return;
}

/*
 * void SpxDisConnReq(queue_t *q, mblk_t *mp)
 *	We got a disconnect request, change the spx packet into a 
 *	T_discon_ind and send it upstream also allocate an 
 *	acknowledge packet and send it.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
/*ARGSUSED*/
void 
SpxDisConnReq(queue_t *q, mblk_t *mp)
{ 	
	register spxConnectionEntry_t *conEntryPtr;
	register spxHdr_t *spxHeader;
	uint destConId;
	mblk_t *spxmp;
	uint16	spxSeqNumber;
	uint16	locAckNumber;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	spxHeader = (spxHdr_t *)mp->b_rptr;
	destConId = PGETINT16(&spxHeader->destinationConnectionId);
	destConId &= CONIDTOINDEX;
	conEntryPtr = &spxConnectionTable[destConId];

	NWSLOG((SPXID, destConId, PNW_ENTER_ROUTINE, SL_TRACE,
			"NSPX: lrp: ENTER DisConReq"));

	if (spxbot == NULL) {
		NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
			"NSPX: lrp: spxbot is NULL"));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}	
	spxSeqNumber = PGETINT16(&spxHeader->sequenceNumber);
	locAckNumber = conEntryPtr->window.local.ack;

	if (spxSeqNumber != locAckNumber) {		/* packet in sequence? */
		NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE, 
			"NSPX: lrp: SpxDisConReq: out of sequence."));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	conEntryPtr->spxHdr.connectionControl = conEntryPtr->protocol; 
	conEntryPtr->window.local.ack++;

	if (!canputnext(spxbot)) {
		NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
			"NSPX: uws: SpxDisConReq cant put down to %Xh",spxbot));
		conEntryPtr->conStats.con_canput_fail++; 
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	if ((spxmp = SpxSetUpPacket(destConId))
		== NULL ) {
		NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: uws: SpxDisConReq cant alloc"));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	spxHeader = (spxHdr_t *)spxmp->b_rptr;
	spxHeader->ipxHdr.len = GETINT16( msgdsize(spxmp) );
	spxHeader->dataStreamType = SPX_TERMINATE_ACK;
	if (conEntryPtr->protocol & SPX_SPXII ) {
		spxHeader->sequenceNumber = (uint16)0;
		spxHeader->connectionControl |=	SPX_EOF_BIT;
	} else
		spxHeader->connectionControl |= SPX_EOF_BIT;	

	putnext(spxbot,spxmp);
	freemsg(mp);

	SpxGenTDis(conEntryPtr, (long) TLI_SPX_CONNECTION_TERMINATED, FALSE);

	NWSLOG((SPXID, destConId, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: lrp: EXIT DisConReq"));
	NTR_VLEAVE();
	return;
}

/*
 * int nspxlrput(queue_t *q, mblk_t *mp)
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int 
nspxlrput(queue_t *q, mblk_t *mp)
{
	register spxHdr_t			*inSpxHdr;
	register spxConnectionEntry_t *conEntryPtr;
	ipxMctl_t *spxMctlPtr;	
	struct iocblk	*iocp;
	uint16 destSocket;
	uint16 sourceSocket;
	uint16 destConId;
	uint8 flushFlags;
	uint32	ipxMaxSDU;
	queue_t	*saveQue;
	uint16	requestedListens;
	int		status;
	uint16	socketMachOrder;
	long	prim;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	if ((q == NULL) || (mp == NULL)) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_ERROR,
			"NSPX: lrp: q or mp is NULL q %Xh mp %Xh",q, mp));
		if (mp != NULL) freemsg(mp);
		return( NTR_LEAVE(0));
	}

	if (q->q_ptr == NULL) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_ERROR,
			"NSPX: lrp: q->q_ptr is NULL"));
		freemsg(mp);
		return( NTR_LEAVE(0));
	}
	
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE, 
		  "NSPX: lrp: ENTER q %Xh",q));

	spxStats.spx_rcv_packet_count++; 
	switch(MTYPE(mp)) {

	case M_DATA :
		if (DATA_SIZE(mp) < SPX_PACKET_HEADER_SIZE) {
			if ( pullupmsg(mp, SPX_PACKET_HEADER_SIZE) == 0 ) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: lrp: BAD size %d packet",DATA_SIZE(mp)));
				spxStats.spx_rcv_bad_packet++; 
				freemsg(mp);
				break;
			}
		}

		inSpxHdr = (spxHdr_t *)mp->b_rptr;

		if (inSpxHdr->ipxHdr.pt != SPX_PACKET_TYPE) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: inbound packet type not spx %Xh",
				inSpxHdr->ipxHdr.pt));
	 		spxStats.spx_rcv_bad_packet++; 
			freemsg(mp);
			break;
		}

		destConId = PGETINT16(&inSpxHdr->destinationConnectionId);

		if ((destConId == SPX_CON_REQUEST_CONID) 
			&& (inSpxHdr->connectionControl & SPX_SYSTEM_PACKET)) {
				spxStats.spx_rcv_conn_req++; 
				SpxConnReq(q,mp);
				break;
		}

		destConId &= CONIDTOINDEX;
		if (destConId >= spxMaxConnections) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: Bad Destination Id %d",destConId));
	 		spxStats.spx_rcv_bad_packet++; 
			freemsg(mp);
			break;
		}

		conEntryPtr = &spxConnectionTable[destConId];
		conEntryPtr->conStats.con_rcv_packet_count++; 

		if (conEntryPtr->upperReadQueue == NULL) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: packet destined for closed connection %d",
				destConId));
	 		spxStats.spx_rcv_bad_packet++; 
			conEntryPtr->conStats.con_rcv_bad_packet++; 
			freemsg(mp);
			break;
		}

		IPXCOPYSOCK(conEntryPtr->spxHdr.ipxHdr.src.sock, &destSocket);
		IPXCOPYSOCK(inSpxHdr->ipxHdr.dest.sock, &sourceSocket);

		if (destSocket != sourceSocket) {
			NWSLOG((SPXID, destConId, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: dest sock N %Xh != stream sock N %Xh",
				destSocket, sourceSocket));
	 		spxStats.spx_rcv_bad_packet++; 
			conEntryPtr->conStats.con_rcv_bad_packet++; 
			freemsg(mp);
			break;
		}
		if (conEntryPtr->pTimeOutId) {
			untimeout(conEntryPtr->pTimeOutId);
			conEntryPtr->pTimeOutId = 0;
		}
	/* spxs heart */
		if (inSpxHdr->connectionControl & SPX_SYSTEM_PACKET) {
			if (inSpxHdr->connectionControl & SPX_ACK_REQUESTED) {
				drv_getparm(LBOLT, (void *)&conEntryPtr->remoteActiveTicks);
				if (conEntryPtr->protocol & SPX_SPXII) {
					if (conEntryPtr->state == TS_WCON_CREQ) {
						SpxSessionSetUp(q,mp); 
						break;
					} else {
						if (inSpxHdr->connectionControl & SPX_NEG) {
							SpxNegotiateReq(q,mp);
							break;
						} 
					}
				}
				/* might be piggy-back ACK on a Watchdog packet */
				/* zero out sequence Number this is a ACK not a NAK */
				conEntryPtr->conStats.con_rcv_watchdog++; 
				inSpxHdr->sequenceNumber = 0;
				SpxAck(q,mp);
				SpxSendAck(destConId, FALSE, 0);
			} else {  /* SYSTEM bit, No ACK_REQUESTED */
				drv_getparm(LBOLT, (void *)&conEntryPtr->remoteActiveTicks);
				conEntryPtr->endTicks = conEntryPtr->remoteActiveTicks;
				conEntryPtr->lastTickDiff = 
					(conEntryPtr->endTicks - conEntryPtr->beginTicks);
				if ((conEntryPtr->state == TS_WCON_CREQ) &&
					(conEntryPtr->spxHdr.destinationConnectionId == 
					SPX_CON_REQUEST_CONID)) {
					SpxConnReqAck(q,mp); 
				} else {
					SpxAck(q,mp);
				}
			}
		} else { 			/* if system packet */
			drv_getparm(LBOLT, (void *)&conEntryPtr->remoteActiveTicks);
			if (conEntryPtr->aTimeOutId) {
				untimeout(conEntryPtr->aTimeOutId);
				conEntryPtr->aTimeOutId = 0;
			}

			switch (inSpxHdr->dataStreamType) {

				case 0:
				default: 	
			 		/* incoming data */
			 		SpxData(q,mp,FALSE); 
					break;
		
				case SPX_TERMINATE_REQUEST:		/* termination indication */
					SpxDisConnReq(q,mp);
					break;

				case SPX_ORDERLY_REL_REQ:		/* Orderly Release Request*/
					SpxOrdRelReq(q,mp);
					break;

				case SPX_TERMINATE_ACK:			/* termination ack */
					SpxDisConnAck(q,mp);
					break;

			} /* dataStreamType switch */

		}
		break;

	case M_FLUSH:
	 	NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_CASE, SL_TRACE, 
			  "NSPX: lrp: M_FLUSH"));
		/*
		 * Flush queues. NOTE: sense of tests is reversed
		 * since we are acting like a "stream head".
		 */

		bcopy((char *)mp->b_rptr, (char *)&flushFlags, sizeof(flushFlags));

		if (flushFlags & FLUSHR)
			flushq(q, FLUSHDATA);
			if (flushFlags & FLUSHW) {
			flushq(WR(q), FLUSHDATA);
			flushFlags &= ~FLUSHR;
			bcopy((char *)&flushFlags, (char *)mp->b_rptr, sizeof(flushFlags));
			qreply(q, mp);
		} else {
			freemsg(mp);
		}
		break;

	case M_IOCTL: 
		if (DATA_SIZE(mp) < sizeof(struct iocblk)) {
			NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: M_IOCTL bad size ioctl %d",DATA_SIZE(mp)));
			freemsg(mp);
			break;
		}
		iocp = (struct iocblk *)mp->b_rptr;
		switch(iocp->ioc_cmd) {
			default : 
	 		NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_DEFAULT, SL_TRACE, 
					  "NSPX: lrp: M_IOCTL DEFAULT %d", iocp->ioc_cmd));
				iocp->ioc_error = EINVAL;

			NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
				"NSPX: uwp: M_IOCTL IOCNAK %d",iocp->ioc_cmd));

			MTYPE(mp) = M_IOCNAK;
			qreply(q,mp);
			break;
		}
		break;

	case M_IOCACK:
		if (DATA_SIZE(mp) < sizeof(struct iocblk)) {
			NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: M_IOCACK bad size ioctl %d",DATA_SIZE(mp)));
			freemsg(mp);
			break;
		}
		iocp = (struct iocblk *)mp->b_rptr;
		prim = T_BIND_REQ;
		switch (iocp->ioc_cmd) {
	 		case IPX_O_BIND_SOCKET:
				prim = O_T_BIND_REQ;
			/*FALLTHRU*/
	 		case IPX_BIND_SOCKET :
	 			NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_CASE, SL_TRACE, 
					"NSPX: lrp: M_IOCACK IPX_BIND_SOCKET"));
				saveQue = (queue_t *)iocp->ioc_id;
				status = iocp->ioc_rval;
				requestedListens = (uint16)iocp->ioc_cr;
				if ((!mp->b_cont) || 
						(DATA_SIZE(mp->b_cont) != sizeof(IpxSetSocket_t))) {
					socketMachOrder = 0;
				} else {
					IPXCOPYSOCK(mp->b_cont->b_rptr, &socketMachOrder);
				}
				freemsg(mp);
				SpxTBindReqCont(saveQue, socketMachOrder, 
						requestedListens, status, prim);
				break;

			default :
	 		NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_DEFAULT, SL_TRACE, 
				"NSPX: lrp: M_IOCACK DEFAULT %d", iocp->ioc_cmd));
				freemsg(mp);
				break; 
			}
		break;

	case M_IOCNAK: 
		if (DATA_SIZE(mp) < sizeof(struct iocblk)) {
			NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
				"NSPX: lrp: M_IOCNAK bad size ioctl %d",DATA_SIZE(mp)));
			freemsg(mp);
			break;
		}
		iocp = (struct iocblk *)mp->b_rptr;
		prim = T_BIND_REQ;
		switch (iocp->ioc_cmd) {
	 		case IPX_O_BIND_SOCKET :
				prim = O_T_BIND_REQ;
			/*FALLTHRU*/
	 		case IPX_BIND_SOCKET :
	 			NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_CASE, SL_TRACE, 
					"NSPX: lrp: M_IOCNAK IPX_BIND_SOCKET"));
				/*
				 * The queue on which the ioctl was issued is
				 * save in ioc_id.  The TPI error is saved in
				 * ioc_error.
				 */
				saveQue = (queue_t *)iocp->ioc_id;
				freemsg(mp);
				SpxGenTErrorAck(saveQue, prim,
				 (long)iocp->ioc_error, 0);
				break;

			default:
	 			NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_DEFAULT, SL_TRACE,
					"NSPX: lrp: M_IOCNAK DEFAULT %d", iocp->ioc_cmd));
				freemsg(mp);
				break;
		}
		break;

	case M_ERROR:
	case M_HANGUP:
	 		NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_CASE, SL_TRACE, 
				"NSPX: lrp: M_ERR M_HNG"));
		spxbot = NULL;
		freemsg(mp);
		break;
		
	case M_PCPROTO:
 		NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_CASE, SL_TRACE, 
			"NSPX: lrp: M_PCPROTO"));
		freemsg(mp);
		break;

	case M_CTL:
		spxMctlPtr = (ipxMctl_t *)mp->b_rptr;	
		switch (spxMctlPtr->mctlblk.cmd) {
			case GET_IPX_INT_NET_NODE:
		 		NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_CASE, SL_TRACE, 
					"NSPX: lrp: M_CTL GET_IPX_INT_NET_NODE"));

				if(spxMctlPtr->mctlblk.u_mctl.ipxInternalNetNode.net != 0) {
					IPXCOPYNET(&spxMctlPtr->mctlblk.u_mctl.ipxInternalNetNode.
						net, &IpxInternalNet);
					IPXCOPYNODE(spxMctlPtr->mctlblk.u_mctl.ipxInternalNetNode.
						node, IpxInternalNode);
					NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE, 
						"NSPX: M_CTL IPX internal net %x",IpxInternalNet));
				} 
#ifdef DEBUG
				 else {
					NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE, 
						"NSPX: M_CTL IPX internal net is ZERO "));
				}
#endif
				freemsg(mp);
				break;

			case SPX_GET_IPX_MAX_SDU:
		 		NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_CASE, SL_TRACE, 
					"NSPX: lrp: M_CTL SPX_GET_IPXMAX_SDU"));
				conEntryPtr = (spxConnectionEntry_t *)
								spxMctlPtr->mctlblk.u_mctl
									.spxGetIpxMaxSDU.conEntry;
				if (conEntryPtr->upperReadQueue == NULL) {
	 				NWSLOG((SPXID,spxMinorDev,PNW_SWITCH_DEFAULT,SL_TRACE,
						"NSPX: lrp: M_MCTL MAX_SDU NULL upperReadQueue"));
					freemsg(mp);
					break;
				}
				if (conEntryPtr->state == TS_WACK_CREQ) {
					SpxTConnReqCont(conEntryPtr,mp);
				}
				else if (conEntryPtr->state == TS_WRES_CIND) {
				/* Get the max SDU from IPX, use this as the starting point
				   for size negotiation.  */ 
					ipxMaxSDU = 
						spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.maxSDU;
					if (ipxMaxSDU < SPX_DEFAULT_PACKET_SIZE)
						ipxMaxSDU = SPX_DEFAULT_PACKET_SIZE;
					else 
						if (ipxMaxSDU > SPX_ABSOLUTE_MAX_PACKET_SIZE) 
							ipxMaxSDU = SPX_ABSOLUTE_MAX_PACKET_SIZE;

					conEntryPtr->maxSDU = 
					conEntryPtr->maxTPacketSize = 
					conEntryPtr->maxRPacketSize = ipxMaxSDU;
					freemsg(mp);
					conEntryPtr->disabled = FALSE;
					enableok(WR(conEntryPtr->upperReadQueue));
					qenable(WR(conEntryPtr->upperReadQueue));
				}
				else if(conEntryPtr->sessionFlags & RE_NEGOTIATE) {
					/* if the maxSDU from IPX is Zero, there is no known route
					   to the other endpoint, so disconnect now.  If a non-zero
					   value is returned, use this value as starting point for
					   re-negotiations.  If a router went down hard, (no
					   broadcast notification) the value might be of the old
					   route.  Ping the connection and get an ACK before 
					   re-negotiating packet size.  */

					ipxMaxSDU = 
						spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.maxSDU;
					if(ipxMaxSDU == 0) {
		 				NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE, 
							"NSPX: M_CTL No new route found, Disconnect"));
						freemsg(mp);
						SpxGenTDis(conEntryPtr, 
							(long) TLI_SPX_CONNECTION_FAILED, FALSE);
					} else {
						if (ipxMaxSDU < SPX_DEFAULT_PACKET_SIZE)
							ipxMaxSDU = SPX_DEFAULT_PACKET_SIZE;
						else 
							if (ipxMaxSDU > SPX_ABSOLUTE_MAX_PACKET_SIZE) 
								ipxMaxSDU = SPX_ABSOLUTE_MAX_PACKET_SIZE;

						conEntryPtr->maxTPacketSize = ipxMaxSDU;
						freemsg(mp);
						destConId = conEntryPtr->spxHdr.sourceConnectionId;
						destConId &= CONIDTOINDEX;
						conEntryPtr->tTicksToWait = 
							conEntryPtr->maxTicksToWait;
						conEntryPtr->pRetries = 0;
						SpxReNegotiate(destConId);
					}
				} else {
	 				NWSLOG((SPXID,spxMinorDev,PNW_SWITCH_DEFAULT,SL_TRACE,
						"NSPX: lrp: M_MCTL MAX_SDU unknown state %d",
						conEntryPtr->state));
					freemsg(mp);
					conEntryPtr->disabled = FALSE;
					enableok(WR(conEntryPtr->upperReadQueue));
					qenable(WR(conEntryPtr->upperReadQueue));
				}
				break;
			default:
	 			NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_DEFAULT, SL_TRACE,
					"NSPX: lrp: M_MCTL DEFAULT %d", spxMctlPtr->mctlblk.cmd));
				freemsg(mp);
				break;
		}
		break;

	default: 	
	 		NWSLOG((SPXID, spxMinorDev, PNW_SWITCH_DEFAULT, SL_TRACE, 
				"NSPX: lrp: DEFAULT mtype %d",MTYPE(mp)));
			spxStats.spx_rcv_bad_packet++; 
			freemsg(mp);
		break;

	} /* MTYPE switch */

	NWSLOG((SPXID, spxMinorDev, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: lrp: EXIT"));

	return( NTR_LEAVE(0));
} /* spxlrput */


/*
 * int nspxursrv(queue_t *q) 
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int 
nspxursrv(queue_t *q) 
{
	mblk_t *mp;
	uint16 conId;
	spxConnectionEntry_t *conEntryPtr;
	long tpiType;
	int	lvl;
	int	cantPut = 0;
	uint32	localAlloc, localAck;

	NTR_ENTER( 1, q, 0, 0, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;
	conId = conEntryPtr->spxHdr.sourceConnectionId;
	conId &= CONIDTOINDEX;

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE, 
		  "NSPX: urs: ENTER q %Xh",q));

	while ((mp = getq(q)) != NULL)  {
		bcopy((char *)mp->b_rptr, (char *)&tpiType, sizeof(tpiType));
		if  ((MTYPE(mp) == M_PROTO) && 
			((tpiType == T_DATA_IND)|| tpiType == T_ORDREL_IND)) {
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: urs: M_PROTO and T_DATA_IND"));
			if (canputnext(q)) {
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: urs: Send data upstream"));
				conEntryPtr->goingUpCount++;
				spxStats.spx_rcv_packet_sentup++; 
				conEntryPtr->conStats.con_rcv_packet_sentup++; 
				putnext(q,mp);
			} else {
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: urs: Still blocked putbq"));
				cantPut++;
				putbq(q,mp);
				break;
			}
		} else {
			NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE,
				"NSPX: urs: DEFAULT mtype %d",MTYPE(mp)));
			freemsg(mp);
		}
	} /* end while getq */
/* set processor level to str this will prevent a meesage coming up stream 
   and being queued after we are out of the while loop.  Only reset 
   flowControl flag when there is no messages on the queue and we 
   canput next.
 */
	lvl = splstr();
	if (conEntryPtr->goingUpCount) {	/* did we do anything */
		if (qsize(q) != 0) {
			if (!cantPut) {		/* if a message slipped after while loop */
				qenable(q);				/* take it off later */
			}
		} else {
			localAlloc = conEntryPtr->window.local.alloc;
			conEntryPtr->window.local.alloc = 
				conEntryPtr->window.local.alloc + conEntryPtr->goingUpCount;
			conEntryPtr->goingUpCount = 0;
			conEntryPtr->flowControl = FALSE;
			/* Send ACK to open window only if window is closed */
			if ((conEntryPtr->window.local.ack < 0x0080) &&
					(localAlloc > 0xff80))  {
				localAck = conEntryPtr->window.local.ack + 0x10000;
			} else {
				localAck = conEntryPtr->window.local.ack;
			}
			if (localAck > localAlloc) {
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: urs: Openwindow Ack=%d Alloc=%d",
					conEntryPtr->window.local.ack,
					conEntryPtr->window.local.alloc));
				SpxSendAck(conId, FALSE, 0);
			}
		}
	}
	splx(lvl);

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, 
				"NSPX: urs: EXIT")); 
	return( NTR_LEAVE(0));
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/spx2wr.c	1.25"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: spx2wr.c,v 1.22.2.1.2.2 1995/02/09 14:59:42 meb Exp $"

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

uint32 	IpxInternalNet = 0;
uint8 	IpxInternalNode[IPX_NODE_SIZE] = {0};

/*
 * int SpxCheckWindow(spxConnectionEntry_t *conEntryPtr)
 *	This routine will do basic checks to determine if a packet can
 *	be sent out on the wire
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int
SpxCheckWindow(spxConnectionEntry_t *conEntryPtr)
{
	int retVal = 0;
	uint32	lRemoteAlloc, lLocalSeq;
	uint16 remoteAlloc, remoteAck, localSeq;

	NTR_ENTER( 1, conEntryPtr, 0, 0, 0, 0);
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: uws: CheckWindow : ENTER "));

	if (conEntryPtr->needAck) {
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: uws: CheckWindow still waiting on ack"));
		retVal = 2;
		goto Exit;
	}

	if (!canputnext(spxbot) ) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: CheckWindow cant put down"));
		conEntryPtr->conStats.con_canput_fail++; 
		retVal = 1;
		goto Exit;
	}

/* If the window is choked  wait until Ack comes in and re-enable the 
   spxuwsrv.
*/
	remoteAlloc = conEntryPtr->window.remote.alloc;
	remoteAck = conEntryPtr->window.remote.ack;
	localSeq = conEntryPtr->window.local.sequence;

	if ((remoteAck >= 0xff80) && (remoteAlloc < 0x0080))
		lRemoteAlloc = (remoteAlloc + 0x10000);
	else 
		lRemoteAlloc = remoteAlloc;

	if ((remoteAck >= 0xff80) && (localSeq < 0x0080))
		lLocalSeq = (localSeq + 0x10000);
	else 
		lLocalSeq = localSeq;

	if (lRemoteAlloc < lLocalSeq) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: CheckWindow window choked win %d seq %d",
				remoteAlloc, localSeq));
		conEntryPtr->conStats.con_window_choke++; 
		retVal = 3;
	}

	Exit:
		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: CheckWindow : EXIT Ret=%x ",retVal));

		return( NTR_LEAVE(retVal));
}

/*
 * long SpxTranData(spxConnectionEntry_t *conEntryPtr, mblk_t *mp, 
 *												uint8 retry, uint8 incSeq)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
long
SpxTranData(spxConnectionEntry_t *conEntryPtr, mblk_t *mp, 
												uint8 retry, uint8 incSeq)
{
	register spxHdr_t *spxHeader;
	mblk_t *spxmp, *spxcopy ;
	long tError = 0; 
	uint16 conId;
	int tickDiff;
	int lastMsg;

	NTR_ENTER( 3, conEntryPtr, mp, retry, 0, 0);
	/* to catch bogus calls */
	if (conEntryPtr == NULL) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: TranData connection is closed"));
		tError = ENOLINK;	/* is this the right error? */
		goto Error;
	}

	/* should never happen checked in uwsrv */
	if (spxbot == NULL) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: TranData link to ipx is broken"));
		tError = ENOLINK;
		goto Error;
	}

	conId = conEntryPtr->spxHdr.sourceConnectionId;
	conId &= CONIDTOINDEX;
	if ((spxmp = SpxSetUpPacket(conId)) == NULL ) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TranData SetUpPacket failed"));
		tError = ENOSR;
		goto Error;
	} 

	spxHeader = (spxHdr_t *)spxmp->b_rptr;
	if (mp != NULL)
		spxHeader->ipxHdr.len = GETINT16( msgdsize(mp) +  msgdsize(spxmp) );
	else
		spxHeader->ipxHdr.len = GETINT16( msgdsize(spxmp) );

	spxmp->b_cont = mp;

	if (retry) {
		if ((spxcopy = dupmsg(spxmp)) == NULL) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uws: TranData cant dupmsg"));
			spxStats.spx_alloc_failures++; 
			freeb(spxmp);
			tError = ENOSR;
			goto Error;
		}

		/* save copy of msg in unacked area, might need to retransmit it */
		lastMsg = (GETINT16(spxHeader->sequenceNumber) %
			 MAX_SPX2_WINDOW_SIZE); 
		conEntryPtr->window.unAckedMsgs[lastMsg] = spxcopy;
		if (spxHeader->connectionControl & SPX_ACK_REQUESTED) {
			conEntryPtr->needAck = TRUE;
			if (conEntryPtr->beginTicks <= conEntryPtr->endTicks)
				tickDiff = conEntryPtr->endTicks - conEntryPtr->beginTicks;
			else
		/* rollover will not happen until 1.3 years without boot*/
				tickDiff = (uint32) 0xFFFFFFFF -
						(conEntryPtr->beginTicks - conEntryPtr->endTicks);

		/* wait last round trip time plus half before retrying */
			tickDiff += (tickDiff >> 1);

			if (tickDiff < conEntryPtr->minTicksToWait) 
				tickDiff = conEntryPtr->minTicksToWait ;
			else
				if (tickDiff > conEntryPtr->maxTicksToWait ) 
					tickDiff = conEntryPtr->maxTicksToWait ;
	
			conEntryPtr->tTicksToWait = tickDiff;
		
			conEntryPtr->window.local.retrySequence = 
					GETINT16(spxHeader->sequenceNumber);
			conEntryPtr->tTimeOutId = itimeout(SpxTTimeOut, (void *)conId,
				tickDiff, pltimeout);
			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_ERROR,
				"TranData: Schedule TTimeOut ID=%x for seq# %d in %d ticks",
				conEntryPtr->tTimeOutId, GETINT16(spxHeader->sequenceNumber),
				tickDiff));
		}
	}
	/* Figure Round Trip Time if Ack Requested, here 
	** because we dont retry watchdogs.
	*/
	if (spxHeader->connectionControl & SPX_ACK_REQUESTED)
		drv_getparm(LBOLT, (void *)&conEntryPtr->beginTicks);

	if (incSeq)
		conEntryPtr->window.local.sequence++;
	spxStats.spx_send_packet_count++; 
	conEntryPtr->conStats.con_send_packet_count++; 
	putnext(spxbot,spxmp);

	return(NTR_LEAVE(tError));

	Error:
		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: EXIT Error TranData %d",tError));

		return(NTR_LEAVE(tError));
}

/*
 * void SpxTDataReq(queue_t *q, mblk_t *mp, int moreData)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxTDataReq(queue_t *q, mblk_t *mp, int moreData)
{
	register spxConnectionEntry_t *conEntryPtr;
	mblk_t *spxmp;
	struct T_data_req *tDataReq;

	NTR_ENTER( 3, q, mp, moreData, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;
	tDataReq = (struct T_data_req *)mp->b_rptr;

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: uws: TDataReq: ENTER "));

	if (tDataReq->MORE_flag) {
		conEntryPtr->spxHdr.connectionControl = 0;
	} else {
		conEntryPtr->spxHdr.connectionControl = SPX_EOF_BIT;
	}
	if (conEntryPtr->protocol & SPX_SPXII) {
	/* set ack requested if this packet will fill the last allocated window,
	   or if this packet will fill our packet save area (re-transmission)
	   or if the the is no more data on our queue
	*/
		if((conEntryPtr->window.remote.alloc == 
			conEntryPtr->window.local.sequence) ||
			(conEntryPtr->window.local.sequence == 
			conEntryPtr->window.remote.ack + MAX_SPX2_WINDOW_SIZE -1) || 
			!(moreData)) {
			conEntryPtr->spxHdr.connectionControl |= SPX_ACK_REQUESTED;
		}
	} else {
		conEntryPtr->spxHdr.connectionControl |= SPX_ACK_REQUESTED;
	}
		
	conEntryPtr->spxHdr.connectionControl |= conEntryPtr->protocol; 
	spxmp = mp->b_cont;
/* 
	If this failes watchEmu will have to ping the connection to 
	reenable the data transfer.
*/
	if (conEntryPtr->specialFlags & SPX_SAVE_SEND_HEADER_FLAG) {
		conEntryPtr->spxHdr.dataStreamType = *spxmp->b_rptr++;
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TDataReq: dataType = %x",
			conEntryPtr->spxHdr.dataStreamType));
	}
	NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX: uws: TDataReq: Sending Packet %d data size %d",
		 conEntryPtr->window.local.sequence, msgdsize(spxmp)));

	if (SpxTranData(conEntryPtr, spxmp, (uint8)TRUE, (uint8)TRUE)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TDataReq: TranData failed "));
		putbq(q,mp);
		NTR_VLEAVE();
		return;
	}
	freeb(mp);

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: uws: TDataReq: EXIT "));
	NTR_VLEAVE();
	return;
}

/*
 * void SpxTData(queue_t *q, mblk_t *mp)
 *	This routine takes a chain of any size T_DATA_REQ msg and dupb it 
 *	up into T_DATA_REQ small enough for the wire.  It will attempt to send out
 *	the smaller packets over the wire. If the connection is in flow control,
 *	it will then put the remaining T_DATA_REQs back on the queue to
 *	be serviced later.. 
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxTData(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;
	uint16 maxDataSize;
	mblk_t *bp, *newMp, *newBp, *freeMp, *markMp;
	struct T_data_req *tDataReq, *tDReq;
	int		windowchoke = 0;
	int		moreData;
	int		specialFlags;
	u_char	dataStreamType;

	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;
	NTR_ENTER( 3, q, mp, conEntryPtr, 0, 0);

	NWSLOG(( SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: uws: TData: ENTER "));

	tDataReq = (struct T_data_req *)mp->b_rptr;

	if ((markMp = allocb(3, BPRI_MED)) == NULL ) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TData cant alloc markMp"));
		putbq(q,mp);
		NTR_VLEAVE();
		return;
	}
	if (conEntryPtr->protocol & SPX_SPXII)
		maxDataSize = (conEntryPtr->maxTPacketSize - SPXII_PACKET_HEADER_SIZE);
	else
		maxDataSize = (conEntryPtr->maxTPacketSize - SPX_PACKET_HEADER_SIZE);
	putbq(q,markMp);	/* Put a maker Mp back on queue for insq */
	bp = mp->b_cont;
	specialFlags = conEntryPtr->specialFlags;
	if (specialFlags & SPX_SAVE_SEND_HEADER_FLAG) {
		dataStreamType = *bp->b_rptr;
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: SpxTData: SPX_SAVE_SEND_HEADER_FLAG set, Save dataType = %x",
			dataStreamType));
	}
	while (bp != NULL) {
		while (DATA_SIZE(bp) > 0 ) {
			if (specialFlags & SPX_SAVE_SEND_HEADER_FLAG) {
				/* If SAVE_HEADER set use copyb will need to change data later*/
				if ((newBp = copyb(bp)) == NULL) {
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
						"NSPX: uws: TData cant copy block "));
					spxStats.spx_alloc_failures++; 
					goto ErrorExit; 
				}
			} else {
				if ((newBp = dupb(bp)) == NULL) {
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
						"NSPX: uws: TData cant dup block "));
					spxStats.spx_alloc_failures++; 
					goto ErrorExit; 
				}
			}
			conEntryPtr->allocRetryCount = 0; 
			if ((uint16)DATA_SIZE(newBp) > maxDataSize) {
				newBp->b_wptr = newBp->b_rptr + maxDataSize;
			}

			if ((newMp = copyb(mp)) ==NULL) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uws: TData cant copy block "));
				spxStats.spx_alloc_failures++; 
				goto ErrorExit;
			}
			tDReq = (struct T_data_req *)newMp->b_rptr;
			tDReq->MORE_flag = TRUE;
			newMp->b_cont = newBp;
			moreData = TRUE;

/* clear MORE falg if last newMp and if orignal mp had MORE flag cleared */
			if ((bp->b_cont == NULL) && 
				((uint16)DATA_SIZE(bp) <= maxDataSize)) {
				if (!tDataReq->MORE_flag) {
					tDReq->MORE_flag = FALSE;
				}
			/* check and see if there is another Message on the q 
			 * (allow for markMp)? If so set flag, so SpxTDataReq won't 
			 * close window
			*/ 
			 	moreData = qsize(q); 
				if (moreData < 3) {
					moreData = 0;
				}
			}
			
			/* If window is closed or has been closed, all msgs goes on queue */
			if ((windowchoke) || (SpxCheckWindow(conEntryPtr))) {
				windowchoke++;
				insq(q,markMp,newMp);
			} else
				SpxTDataReq(q,newMp,moreData);

			if ((uint16)DATA_SIZE(bp) > maxDataSize) {
				if (specialFlags & SPX_SAVE_SEND_HEADER_FLAG) {
					bp->b_rptr = (bp->b_rptr + maxDataSize - 1);
					*bp->b_rptr = dataStreamType;
				} else 
					bp->b_rptr = bp->b_rptr + maxDataSize;
			} else
				 bp->b_wptr = bp->b_rptr;
		}
		freeMp = bp;
		bp = bp->b_cont;
		mp->b_cont = bp;		/* keep mp current for error recovery */
		freeb(freeMp);
	}

	rmvq(q,markMp);
	freemsg(markMp);
	freemsg(mp);

	qenable(q);

	NWSLOG(( SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: uws: TData: EXIT "));
	NTR_VLEAVE();
	return;

	ErrorExit:
		if (conEntryPtr->allocRetryCount >= spxMaxAllocRetries) {	
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uws: TData reached max alloc retries %d",
				spxMaxAllocRetries));
			SpxGenTDis(conEntryPtr, (uint8)TLI_SPX_CONNECTION_FAILED, TRUE);
		} else {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uws: TData requeueing messages %X", mp));
			insq(q,markMp,mp);		/* Put remaining mp back on que for later*/
			conEntryPtr->allocRetryCount++;
		}

		rmvq(q,markMp);
		freemsg(markMp);
		NWSLOG(( SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: uws: TData: EXIT "));
		NTR_VLEAVE();
		return;
}

/*
 * mblk_t * SpxMData(queue_t *q, mblk_t *mp, spxConnectionEntry_t *conEntryPtr)
 *	This routine takes a chain of any size M_DATA msg and dupb it 
 *	up into M_DATA small enough for spx and then puts a T_DATA_REQs
 *	header in front. It then puts this data_req back on the queue to
 *	be serviced. 
 *	
 *	Modified to put the msg back at the begining of the queue, putq would
 *	put the msg at the end of the queue.  The data might be out of order if
 *	aonther msg was on the queue.  Put a maker back (front) on the queue 
 *	using putbq.  Then use insq to put the mp in the correct order.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
mblk_t *
SpxMData(queue_t *q, mblk_t *mp, spxConnectionEntry_t *conEntryPtr)
{
	uint16 maxDataSize;
	mblk_t *tdMp, *newMp=NULL, *freeMp, *markMp;
	struct T_data_req *tDataReq;
	int specialFlags;
	u_char dataStreamType;

	NTR_ENTER( 3, q, mp, conEntryPtr, 0, 0);

	NWSLOG(( SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: MData: Enter q %Xh mp %Xh conEntryPtr %Xh",q,mp,conEntryPtr));

	tDataReq = (struct T_data_req *)NULL;

	if ((markMp = allocb(10, BPRI_MED)) == NULL ) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: MData cant alloc markMp"));
		putbq(q,mp);
		return( (mblk_t *)NTR_LEAVE((int)mp));
	}
	if (conEntryPtr->protocol & SPX_SPXII)
		maxDataSize = (conEntryPtr->maxTPacketSize - SPXII_PACKET_HEADER_SIZE);
	else
		maxDataSize = (conEntryPtr->maxTPacketSize - SPX_PACKET_HEADER_SIZE);
	putbq(q,markMp);	/* Put a maker Mp back on queue for insq */
	specialFlags = conEntryPtr->specialFlags;
	if (specialFlags & SPX_SAVE_SEND_HEADER_FLAG) {
		dataStreamType = *mp->b_rptr;
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: SpxMData: SPX_SAVE_SEND_HEADER_FLAG set, Save dataType = %x",
			dataStreamType));
	}
	while (mp != NULL) {

		while (DATA_SIZE(mp) > 0 ) {

			if (specialFlags & SPX_SAVE_SEND_HEADER_FLAG) {
			/* If SAVE_HEADER set use copyb will need to change data later*/
				if ((newMp = copyb(mp)) == NULL) {
					spxStats.spx_alloc_failures++; 
					goto ErrorExit; 
				}
			} else {
				if ((newMp = dupb(mp)) == NULL) {
					spxStats.spx_alloc_failures++; 
					goto ErrorExit; 
				}
			}
			if ((uint16)DATA_SIZE(newMp) > maxDataSize) 
				newMp->b_wptr = newMp->b_rptr + maxDataSize;

			if ((tdMp = allocb(sizeof(struct T_data_req), BPRI_MED)) ==NULL) {
				spxStats.spx_alloc_failures++; 
				goto ErrorExit;
			}
			MTYPE(tdMp) = M_PROTO;
			tdMp->b_wptr = tdMp->b_rptr + sizeof(struct T_data_req);
			tDataReq = (struct T_data_req *)tdMp->b_rptr;
			tDataReq->PRIM_type = T_DATA_REQ;
			tDataReq->MORE_flag = TRUE;
			tdMp->b_cont = newMp;

			insq(q, markMp, tdMp);

			if ((uint16)DATA_SIZE(mp) > maxDataSize) {
				if (specialFlags & SPX_SAVE_SEND_HEADER_FLAG) {
					mp->b_rptr = (mp->b_rptr + maxDataSize - 1);
					*mp->b_rptr = dataStreamType;
				} else
					mp->b_rptr = mp->b_rptr + maxDataSize;
			} else 
				mp->b_wptr = mp->b_rptr;
		}
		freeMp = mp;
		mp = mp->b_cont;
		freeb(freeMp);
	}

/* set the last one to be end of message */
	if (tDataReq)
		tDataReq->MORE_flag = FALSE;
	rmvq(q,markMp);
	freemsg(markMp);

	qenable(q);

	NWSLOG(( SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: MData: Exit"));
	return( (mblk_t *)NTR_LEAVE(NULL));

	ErrorExit:
		insq(q, markMp, mp);
		rmvq(q,markMp);
		freemsg(markMp);
		if (newMp)
			freemsg(newMp);
		NWSLOG(( SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: MData: Error Exit"));
		return( (mblk_t *)NTR_LEAVE((int)mp));
}

/*
 * void SpxSaveSendHeader(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxSaveSendHeader(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocp;
	spxConnectionEntry_t *conEntryPtr;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;

	if (DATA_SIZE(mp) < sizeof(struct iocblk)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uwp: SaveSendHeader: M_IOCTL bad size %d",DATA_SIZE(mp)));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	iocp = (struct iocblk *)mp->b_rptr;

	if (iocp->ioc_cmd != SPX_GS_DATASTREAM_TYPE) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uwp: SaveSendHeader: M_IOCTL bad cmd %d",iocp->ioc_cmd));
		iocp->ioc_error = EINVAL;
		goto iocnak;
	}

	conEntryPtr->specialFlags |= SPX_SAVE_SEND_HEADER_FLAG;

	MTYPE(mp) = M_IOCACK;
	qreply(q,mp);
	NTR_VLEAVE();
	return;

	iocnak:
		MTYPE(mp) = M_IOCNAK;
		qreply(q,mp);
		NTR_VLEAVE();
		return;
}

/*
 * void SpxMaxPacket(queue_t *q, mblk_t *mp )
 *	This function was changed in spxII to return the maxPacketSize after size
 *	negotiation.  Since this ioctl is only valid in TS_DATA_XFER state size
 *	negotiation will have occured (if connection is spxII).
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void 
SpxMaxPacket(queue_t *q, mblk_t *mp )
{
	uint16 maxDataSize;
	struct iocblk *iocp;
	spxConnectionEntry_t *conEntryPtr;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;

	if (DATA_SIZE(mp) < sizeof(struct iocblk)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uwp: GetMaxPacket: M_IOCTL bad size %d",DATA_SIZE(mp)));
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}

	iocp = (struct iocblk *)mp->b_rptr;

	if (iocp->ioc_cmd != SPX_GS_MAX_PACKET_SIZE) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uwp: GetMaxPacket: M_IOCTL bad cmd %d",iocp->ioc_cmd));
		iocp->ioc_error = EINVAL;
		goto iocnak;
	}

	if (conEntryPtr->state != TS_DATA_XFER) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uwp: GetMaxPacket: M_IOCTL bad state %d",
			conEntryPtr->state));
		iocp->ioc_error = ENONET;
		goto iocnak;
	}

	if (mp->b_cont == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uwp: GetMaxPacket: b_cont is NULL"));
		iocp->ioc_error = EINVAL;
		goto iocnak;
	}

	if (DATA_SIZE(mp->b_cont) < sizeof(uint32)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uwp: GetMaxPacket: b_cont msg size too small %d ",
				DATA_SIZE(mp->b_cont) ));
		iocp->ioc_error = EINVAL;
		goto iocnak;
	}
	if (conEntryPtr->protocol & SPX_SPXII)
		maxDataSize = (conEntryPtr->maxTPacketSize - SPXII_PACKET_HEADER_SIZE);
	else
		maxDataSize = (conEntryPtr->maxTPacketSize - SPX_PACKET_HEADER_SIZE);

	NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX: GetMaxPacket: returning size %d",maxDataSize));

	MTYPE(mp) = M_IOCACK;
	bcopy((char *)&maxDataSize, (char *)mp->b_cont->b_rptr,sizeof(maxDataSize));
	qreply(q,mp);

	NTR_VLEAVE();
	return;

	iocnak:
		MTYPE(mp) = M_IOCNAK;
		qreply(q,mp);
		NTR_VLEAVE();
		return;
}

/*
 * void SpxGenTErrorAck(queue_t *q, long primType, long tError, long tUnixError)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxGenTErrorAck(queue_t *q, long primType, long tError, long tUnixError)
{
	mblk_t *mp;
	struct T_error_ack *tErrorAck;
	spxConnectionEntry_t *conEntryPtr;

	NTR_ENTER( 4, q, primType, tError, tUnixError, 0);

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"NSPX: GenTErrorAck : ENTER q %Xh",q));

	if (q == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: GenTErrorAck q is NULL"));
		NTR_VLEAVE();
		return;
	}

	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;

	if ((conEntryPtr < &spxConnectionTable[0]) ||
		(conEntryPtr > &spxConnectionTable[spxMaxConnections-1])) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: GenTErrorAck: conEntryPtr invalid range"));
		NTR_VLEAVE();
		return;
	}

	if (conEntryPtr->upperReadQueue == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: GenTErrorAck: connection is closed "));
		NTR_VLEAVE();
		return;
	}
		
	NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX: GenTErrorAck : prim %d terror %d errno %d",
			primType, tError, tUnixError));

	if ((mp=allocb(sizeof(struct T_error_ack),BPRI_HI)) == NULL) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: GenTErrorAck : could alloc %d at hipri",
			sizeof(struct T_error_ack)));
		NTR_VLEAVE();
		return;
	}
			
	tErrorAck = (struct T_error_ack *)mp->b_rptr;
	tErrorAck->PRIM_type = T_ERROR_ACK;
	tErrorAck->ERROR_prim = primType;
	tErrorAck->TLI_error = tError;
	tErrorAck->UNIX_error = tUnixError;
	MTYPE(mp) = M_PCPROTO;
	mp->b_wptr = mp->b_rptr + sizeof(struct T_error_ack);
	qreply(q,mp);

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: GenTErrorAck: EXIT "));
	NTR_VLEAVE();
	return;
}

/*
 * void SpxTInfoReq(queue_t *q)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxTInfoReq(queue_t *q)
{
	register spxConnectionEntry_t *conEntryPtr;
	struct T_info_ack *tpiPrimPtr;
	long tError, tUnixError;
	mblk_t *nmp;
	int		msgsz;

	NTR_ENTER( 1, q, 0, 0, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;
		
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE, 
		"NSPX: uwp: ENTER TInfoReq"));

	/* since the message T_info_req is not big enough to reply with we
	   need to allocate memory */

	if ((nmp = allocb(sizeof(struct T_info_ack),BPRI_MED)) == NULL) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: uwp: TInfoReq alloc failed %d",
			sizeof(struct T_info_ack)));
		tError = TSYSERR;
		tUnixError = ENOSR;
		goto ErrorAck;
	}

	if (strmsgsz == 0)
		msgsz = 65535;		/* if unlimited, set TIDU high */
	else
		msgsz = strmsgsz;

	NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX: TInfoReq: strmsgsz = %d set msgsz to %d",
		strmsgsz, msgsz));

	tpiPrimPtr 	= (struct T_info_ack *)nmp->b_rptr;
	tpiPrimPtr->PRIM_type 	= T_INFO_ACK;
	tpiPrimPtr->TSDU_size 	= SPX_TSDU_SIZE;
	tpiPrimPtr->ETSDU_size 	= SPX_ETSDU_SIZE;
	tpiPrimPtr->CDATA_size 	= SPX_CDATA_SIZE;
	tpiPrimPtr->DDATA_size 	= SPX_DDATA_SIZE;
	tpiPrimPtr->ADDR_size 	= sizeof(ipxAddr_t);
	tpiPrimPtr->TIDU_size 	= msgsz; 
	tpiPrimPtr->CURRENT_state = conEntryPtr->state;
	if (conEntryPtr->spx2Options)
		tpiPrimPtr->OPT_size = sizeof(SPX2_OPTIONS);
	else
		tpiPrimPtr->OPT_size = sizeof(SPX_OPTMGMT);
	if (conEntryPtr->protocol & SPX_SPXII)
		tpiPrimPtr->SERV_type = T_COTS_ORD;
	else
		tpiPrimPtr->SERV_type = T_COTS;

#ifndef OS_AIX
	tpiPrimPtr->PROVIDER_flag = XPG4_1 | SNDZERO;
#endif

	MTYPE(nmp) = M_PCPROTO;
	nmp->b_wptr = nmp->b_rptr+sizeof(struct T_info_ack);

	qreply(q,nmp);

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, 
		"NSPX: uwp: EXIT TInfoReq"));

	NTR_VLEAVE();
	return;

	ErrorAck:
		SpxGenTErrorAck(q, (long) T_INFO_REQ, tError, tUnixError); 

		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, 
			"NSPX: uwp: EXIT Error TInfoReq"));
		NTR_VLEAVE();
		return;
}

#ifndef NW_UP
/*
 * void SpxTAddrReq(queue_t *q)
 *	Respond to TPI request for the end point's address information
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 */
void
SpxTAddrReq(queue_t *q)
{
	register spxConnectionEntry_t *conEntryPtr;
	struct T_addr_ack *tpiPrimPtr;
	mblk_t *nmp;
	ipxAddr_t *ipxAddrPtr;
	int allocsize = sizeof(struct T_addr_ack);
	long LOCADDR_length = 0;
	long LOCADDR_offset = 0;
	long REMADDR_length = 0;
	long REMADDR_offset = 0;

	NTR_ENTER( 1, q, 0, 0, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;
		
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE, 
		"NSPX: uwp: ENTER TInfoReq"));

	/*
	 * Since the message T_addr_req is not big enough to reply with we
	 * need to allocate memory for the T_addr_ack.  If endpoint is bound
	 * (i.e. its TPI state is TS_IDLE) also need to include size of an
	 * ipxAddr_t to return the address information of the local address.
	 * If endpoint is connected (i.e. its TPI state is TS_DATAXFER) also
	 * need to include size of another ipxAddr_t to return the remote
	 * address information (i.e. the connection's peer).
	 */
	if (conEntryPtr->state == TS_DATA_XFER) {
		LOCADDR_length = sizeof(ipxAddr_t);
		REMADDR_length = sizeof(ipxAddr_t);
		LOCADDR_offset = sizeof(struct T_addr_ack);
		REMADDR_offset = LOCADDR_offset + LOCADDR_length;
		allocsize += sizeof(ipxAddr_t) * 2;
	} else if (conEntryPtr->state != TS_UNBND) {
		LOCADDR_length = sizeof(ipxAddr_t);
		LOCADDR_offset = sizeof(struct T_addr_ack);
		allocsize += sizeof(ipxAddr_t);
	}
	if ((nmp = allocb(allocsize,BPRI_MED)) == NULL) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: uwp: TInfoReq alloc failed %d", allocsize));
		SpxGenTErrorAck(q, (long) T_ADDR_REQ, TSYSERR, ENOSR); 

		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, 
			"NSPX: uwp: EXIT Error TAddrReq"));
		NTR_VLEAVE();
		return;
	}

	tpiPrimPtr 	= (struct T_addr_ack *)nmp->b_rptr;
	tpiPrimPtr->PRIM_type 	= T_ADDR_ACK;
	tpiPrimPtr->LOCADDR_length = LOCADDR_length;
	tpiPrimPtr->LOCADDR_offset = LOCADDR_offset;
	tpiPrimPtr->REMADDR_length = REMADDR_length;
	tpiPrimPtr->REMADDR_offset = REMADDR_offset;
	if (conEntryPtr->state == TS_DATA_XFER) {
		ipxAddrPtr = (ipxAddr_t *)(nmp->b_rptr +
		 tpiPrimPtr->REMADDR_offset);
		IPXCOPYADDR(&(conEntryPtr->spxHdr.ipxHdr.dest), ipxAddrPtr);
	}
	if (conEntryPtr->state != TS_UNBND) {
		ipxAddrPtr = (ipxAddr_t *)(nmp->b_rptr +
		 tpiPrimPtr->LOCADDR_offset);
		IPXCOPYADDR(&(conEntryPtr->spxHdr.ipxHdr.src), ipxAddrPtr);
	}

	MTYPE(nmp) = M_PCPROTO;
	nmp->b_wptr = nmp->b_rptr + allocsize;

	qreply(q,nmp);

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, 
		"NSPX: uwp: EXIT TAddrReq"));

	NTR_VLEAVE();
	return;
}
#endif /* NW_UP */

/*
 * void SpxRelIpxSocket(uint16 socketMachOrder)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxRelIpxSocket(uint16 socketMachOrder)
{
	mblk_t			*mp, *nmp;
	struct iocblk	*iocp;

	NTR_ENTER( 1, socketMachOrder, 0, 0, 0, 0);

    if ((mp = allocb(sizeof(struct iocblk), BPRI_MED)) == NULL) {
		NWSLOG((SPXID,0,PNW_ERROR,SL_TRACE,
			"SpxRelIpxSocket: allocb failed %d", sizeof(struct iocblk)));
		spxStats.spx_alloc_failures++; 
		NTR_VLEAVE();
		return;
    }
    if ((nmp = allocb(sizeof(IpxSetSocket_t), BPRI_MED)) == NULL) {
		NWSLOG((SPXID,0,PNW_ERROR,SL_TRACE,
			"SpxRelIpxSocket: allocb failed %d", sizeof(IpxSetSocket_t)));
		spxStats.spx_alloc_failures++; 
		NTR_VLEAVE();
		return;
	}
	MTYPE(mp) = M_IOCTL;
	iocp = (struct iocblk *)mp->b_rptr;
    iocp->ioc_cmd = IPX_UNBIND_SOCKET;
    iocp->ioc_count = sizeof(IpxSetSocket_t);
	IPXCOPYSOCK(&socketMachOrder, nmp->b_rptr);
    nmp->b_wptr = nmp->b_rptr + sizeof(IpxSetSocket_t);
    mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);
    mp->b_cont = nmp;

    NWSLOG((SPXID,0,PNW_ALL,SL_TRACE,
        "SpxRelIpxSocket: Request socket 0x%X", socketMachOrder));
	if(spxbot)
		putnext(spxbot,mp);
#ifdef DEBUG
	else
		NWSLOG((SPXID,0,PNW_ERROR,SL_TRACE,
			"SpxRelIpxSocket: spxbot is NULL"));
#endif
	NTR_VLEAVE();
	return;
}

/*
 * int SpxGetIpxSocket(queue_t *q, uint16 socketMachOrder, uint16 requestedListens, int tli_flag)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int
SpxGetIpxSocket(queue_t *q, uint16 socketMachOrder, uint16 requestedListens, int tli_flag)
{
	mblk_t			*mp, *nmp;
	struct iocblk	*iocp;

	NTR_ENTER( 4, q, socketMachOrder, requestedListens, tli_flag, 0);

    if ((mp = allocb(sizeof(struct iocblk), BPRI_MED)) == NULL) {
		NWSLOG((SPXID,0,PNW_ERROR,SL_TRACE,
			"SpxGetIpxSocket: allocb failed %d", sizeof(struct iocblk)));
		spxStats.spx_alloc_failures++; 
        return(NTR_LEAVE(-1));
    }
    if ((nmp = allocb(sizeof(IpxSetSocket_t), BPRI_MED)) == NULL) {
		NWSLOG((SPXID,0,PNW_ERROR,SL_TRACE,
			"SpxGetIpxSocket: allocb failed %d", sizeof(IpxSetSocket_t)));
		spxStats.spx_alloc_failures++; 
        return(NTR_LEAVE(-1));
	}
	MTYPE(mp) = M_CTL;
	MTYPE(nmp) = M_PROTO;
	iocp = (struct iocblk *)mp->b_rptr;
	if ( tli_flag ) {
		iocp->ioc_cmd = IPX_O_BIND_SOCKET;
	} else {
		iocp->ioc_cmd = IPX_BIND_SOCKET;
	}
    iocp->ioc_count = sizeof(IpxSetSocket_t);
	/*
	** save que pointer in id field needed after return
	** save requested listens in other unused field (cred or uid)
	*/
    iocp->ioc_id = (uint)q;
    iocp->ioc_cr = (void *)requestedListens;
	IPXCOPYSOCK(&socketMachOrder, nmp->b_rptr);

    nmp->b_wptr = nmp->b_rptr + sizeof(IpxSetSocket_t);
    mp->b_wptr = mp->b_rptr + sizeof(struct iocblk);
    mp->b_cont = nmp;

	if(spxbot) {
		NWSLOG((SPXID,0,PNW_ALL,SL_TRACE,
			"SpxGetIpxSocket: Request socket 0x%X", socketMachOrder));
		putnext(spxbot,mp);
		return( NTR_LEAVE(0));
	} else {
		NWSLOG((SPXID,0,PNW_ERROR,SL_TRACE,
			"SpxGetIpxSocket: spxbot is NULL"));
        return(NTR_LEAVE(-1));
	}
}

/*
 * void SpxTBindReq(queue_t *q, mblk_t *mp, int tli_flag)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxTBindReq(queue_t *q, mblk_t *mp, int tli_flag)
{
	register spxConnectionEntry_t *conEntryPtr;
	register struct T_bind_req *tBindReq;
	uint16 socketNetOrder;
	uint16 socketMachOrder;
	uint16 *socketPtr;
	long bindReqError = 0;
	long bindReqUnixError = 0;
	uint32 requestedListens;

	NTR_ENTER( 3, q, mp, tli_flag, 0, 0);
	tBindReq = (struct T_bind_req *)mp->b_rptr;
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;
	bindReqError =0;
	bindReqUnixError =0;

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"NSPX: uws: TBindReq ENTER "));

	if (spxbot == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TBindReq: Link to IPX broken"));
		bindReqError = TSYSERR;
		bindReqUnixError = EUNATCH;
		goto ErrorAck;
	}
	
	if (DATA_SIZE(mp) < sizeof(struct T_bind_req)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TBindReq: bad header size %d",DATA_SIZE(mp)));
		bindReqError = TSYSERR;
		bindReqUnixError = EBADMSG;
		goto ErrorAck;
	}

	if (conEntryPtr->state != TS_UNBND ) { 
		bindReqError = TOUTSTATE;
		goto ErrorAck;  
	}

	if (tBindReq->ADDR_length ==  0 ) {
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: uws: TBindReq: addr length=0 (dynamic socket)"));
		socketNetOrder = 0;
	} else  {
		if (tBindReq->ADDR_length == sizeof(ipxAddr_t)) {
			socketPtr = (uint16 *)(mp->b_rptr + tBindReq->ADDR_offset 
				+ IPX_NET_SIZE + IPX_NODE_SIZE);
			IPXCOPYSOCK(socketPtr,&socketNetOrder);
		} else {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uws: TBindReq bad bind address size %d",
				tBindReq->ADDR_length));
			bindReqError = TBADADDR;
			goto ErrorAck;
		}
	}

	requestedListens = tBindReq->CONIND_number;
	if (requestedListens > spxMaxListensPerSocket ) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TBindReq: requested qlen %d set to max %d",
				requestedListens, spxMaxListensPerSocket));
		requestedListens = spxMaxListensPerSocket; 
	}
	spxStats.spx_listen_req += requestedListens; 

	socketMachOrder = GETINT16(socketNetOrder);

	NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX: uws: TBindReq: socket requested M %Xh",
		GETINT16((uint)socketMachOrder)));
	NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX: uws: TBindReq: socket requested N %Xh",
		GETINT16((uint)socketNetOrder)));

	if (SpxGetIpxSocket(q, socketMachOrder, requestedListens, tli_flag)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TBindReq: SpxGetIpxSocket failed"));
		bindReqError = TSYSERR;
		bindReqUnixError = ENOSR;
		goto ErrorAck;
	}
	freemsg(mp);
	NTR_VLEAVE();
	return;

	ErrorAck:
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: TBindReq: Error Ack terror %d errno %d",
			bindReqError, bindReqUnixError));
		freemsg(mp);
		if (tli_flag) {
			SpxGenTErrorAck(q, (long) O_T_BIND_REQ,
				bindReqError, bindReqUnixError); 
		} else {
			SpxGenTErrorAck(q, (long) T_BIND_REQ,
				bindReqError, bindReqUnixError); 
		}
		NTR_VLEAVE();
		return;
} /* SpxTBindReq */

/*
 * void SpxTBindReqCont(queue_t *q, uint16 socketMachOrder, uint16 requestedListens, int ipxSockStatus, long primitive)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxTBindReqCont(queue_t *q, uint16 socketMachOrder, 
					uint16 requestedListens, int ipxSockStatus, long primitive)
{
	register spxConnectionEntry_t *conEntryPtr;
	struct T_bind_ack *tBindAck;
	uint16		socketNetOrder;
	long		bindReqError = 0;
	long		bindReqUnixError = 0;
	int			index, co;
	int			changeState = 0;
	int			listener = 0;
	mblk_t		*mp = NULL;

	NTR_ENTER(5,q,socketMachOrder,requestedListens,ipxSockStatus,primitive);

	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;
#ifdef DEBUG
	if (socketMachOrder == 0) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: TBindReqCont: no socket from IPX"));
		bindReqError = TNOADDR;
		goto ErrorAck;
	}
#endif
	socketNetOrder = GETINT16(socketMachOrder);
	if ( ipxSockStatus < 0) {
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX BindReqCont: ipxSockStatus= -1, req sock 0x%X already bound",
				socketMachOrder));
		if (requestedListens) {
			for (index=0; index < spxSocketCount; index++) {
				if (spxSocketTable[index].socketNumber == socketNetOrder)
					break;
			}

			if ( index != spxSocketCount ) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX BindReqCont: req sock already bound with listeners"));
				bindReqError = -ipxSockStatus;
				goto ErrorAck;
			}
		}
	}
	NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX: TBindReqCont: socket granted Net %Xh",
		GETINT16((uint)socketNetOrder)));
/*
	alloc a slot and set up the listens
*/
	if (requestedListens > (uint16)0) {
		for (index=0; index < spxSocketCount; index++) 
			if (spxSocketTable[index].socketNumber == 0)
				break;

		if ( index >= spxSocketCount ) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: TBindReqCont: no free slot socket table"));
			bindReqError = TSYSERR;
			bindReqUnixError = ENOSR;
	 		spxStats.spx_listen_req_fails++; 
			goto ErrorAck; 
		}

		for (co=0; co<spxMaxListensPerSocket; co++) 
			spxSocketTable[index].listens[co].acked = TRUE;

		spxSocketTable[index].socketNumber = socketNetOrder;
		spxSocketTable[index].listenConnectionId = 
							conEntryPtr->spxHdr.sourceConnectionId;
		spxSocketTable[index].postedListens = requestedListens ;
		listener = 1;
	}

/* set current state for this endpoint and its socket */
	conEntryPtr->state = TS_IDLE;
	changeState = 1;
	IPXCOPYNET(&IpxInternalNet, conEntryPtr->spxHdr.ipxHdr.src.net);
	IPXCOPYNODE(IpxInternalNode, conEntryPtr->spxHdr.ipxHdr.src.node);
	IPXCOPYSOCK(&socketNetOrder,conEntryPtr->spxHdr.ipxHdr.src.sock);

	if ((mp=allocb(sizeof(struct T_bind_ack) + sizeof(ipxAddr_t),
		BPRI_MED)) == NULL) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: TBindReqCont: bind_ack alloc failed %d",
			sizeof(struct T_bind_ack) + sizeof(ipxAddr_t)));
		bindReqError = TSYSERR;
		bindReqUnixError = ENOSR;
		goto ErrorAck;
	}
				
	MTYPE(mp) = M_PROTO;	
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_rptr + sizeof(struct T_bind_ack) 
		+ sizeof(ipxAddr_t);
	tBindAck = (struct T_bind_ack *)mp->b_rptr;
	tBindAck->PRIM_type = T_BIND_ACK;
	tBindAck->CONIND_number = (uint32)requestedListens;
	tBindAck->ADDR_length = sizeof(ipxAddr_t);
	tBindAck->ADDR_offset = sizeof(struct T_bind_ack);

	/* copy local net and node before returning */

	IPXCOPYNET(&IpxInternalNet, (mp->b_rptr + tBindAck->ADDR_offset));
	IPXCOPYNODE(IpxInternalNode, (mp->b_rptr + tBindAck->ADDR_offset +
		IPX_NET_SIZE));
	IPXCOPYSOCK(&socketNetOrder,((uint8 *)tBindAck + tBindAck->ADDR_offset
		+ IPX_NET_SIZE + IPX_NODE_SIZE ));
		
	qreply(q,mp);

	NTR_VLEAVE();
	return;

	ErrorAck:
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: TBindReqCont: Error Ack terror %d errno %d",
			bindReqError, bindReqUnixError));
		if(socketMachOrder) {
			SpxRelIpxSocket(socketMachOrder);
		}
		if (changeState) {
			conEntryPtr->state = TS_UNBND;
			conEntryPtr->spxHdr.ipxHdr.src.sock[0] = 0;
			conEntryPtr->spxHdr.ipxHdr.src.sock[1] = 0;
		}
		if (listener) {
			bzero((char *)&spxSocketTable[index],sizeof(spxSocketEntry_t));
		}
		if(mp)
			freemsg(mp);
		SpxGenTErrorAck(q, primitive, bindReqError, bindReqUnixError); 
		NTR_VLEAVE();
		return;
} /* SpxTBindReqCont */

/*
 * int SpxTSetOptmgmt(spxConnectionEntry_t *conEntryPtr, void *optReq, int negFlag)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int
SpxTSetOptmgmt(spxConnectionEntry_t *conEntryPtr, void *optReq, int negFlag)
{
	SPX2_OPTIONS *spx2OptReq;
	SPX_OPTMGMT *spxOptReq;
	int		rtval = 0;
	uint32	minDeltaTicks, maxDeltaTicks;

	NTR_ENTER( 3, conEntryPtr, optReq, 0, 0, 0);
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: ENTER TSetOptmgmt"));

	if (conEntryPtr->spx2Options) {
		spx2OptReq = optReq;
		if ((spx2OptReq->versionNumber > OPTIONS_VERSION) ||
			(spx2OptReq->versionNumber == 0)) {
			rtval = 1;
			goto Exit;
		}
		if (negFlag) {
			if (!(spx2OptReq->spxIISessionFlags & 
					SPX_SF_IPX_CHECKSUM)) {
				conEntryPtr->ipxChecksum = FALSE;
				NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				  "NSPX:TSetOptmgmt:Set Checksums OFF"));
			} else {
				conEntryPtr->ipxChecksum = TRUE;
				NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				  "NSPX:TSetOptmgmt:Set Checksums ON"));
			}

			if (spx2OptReq->spxIIOptionNegotiate == 
					SPX2_NO_NEGOTIATE_OPTIONS) {
				conEntryPtr->protocol &= ~SPX_NEG;
				/* if No negotiate, can not have checksums.*/
				conEntryPtr->ipxChecksum = FALSE;
				NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				  "NSPX:TSetOptmgmt:Set Negotiate OFF"));
			} else {
				spx2OptReq->spxIIOptionNegotiate = SPX2_NEGOTIATE_OPTIONS;
				conEntryPtr->protocol |= SPX_NEG;
				NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				  "NSPX:TSetOptmgmt:Set Negotiate ON"));
			}
		}

		if (spx2OptReq->spxIIRetryCount < MIN_SPX_TRETRIES_COUNT) 
			spx2OptReq->spxIIRetryCount = MIN_SPX_TRETRIES_COUNT;
		else if (spx2OptReq->spxIIRetryCount > MAX_SPX_TRETRIES_COUNT)
			spx2OptReq->spxIIRetryCount = MAX_SPX_TRETRIES_COUNT;

		conEntryPtr->tMaxRetries = spx2OptReq->spxIIRetryCount;
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TSetOptmgmt: Set Retry Count to %d",
				spx2OptReq->spxIIRetryCount));

		if (spx2OptReq->spxIIMinimumRetryDelay == 0 ) 
			spx2OptReq->spxIIMinimumRetryDelay = SPX2_MIN_RETRY_DELAY;
		else if (spx2OptReq->spxIIMinimumRetryDelay < MIN_SPX2_MIN_RETRY_DELAY) 
			spx2OptReq->spxIIMinimumRetryDelay = MIN_SPX2_MIN_RETRY_DELAY;
		else if (spx2OptReq->spxIIMinimumRetryDelay > MAX_SPX2_MIN_RETRY_DELAY) 
			spx2OptReq->spxIIMinimumRetryDelay = MAX_SPX2_MIN_RETRY_DELAY;

		conEntryPtr->minTicksToWait = 
			MSEC2TCKS(spx2OptReq->spxIIMinimumRetryDelay);
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TSetOptmgmt: Set Min Retry Delay to %d ticks",
				conEntryPtr->minTicksToWait));

		/* Convert all values to Ticks */
		minDeltaTicks = MIN_SPX2_MAX_RETRY_DELTA * HZ;
		maxDeltaTicks = MAX_SPX2_MAX_RETRY_DELTA * HZ;
		spx2OptReq->spxIIMaximumRetryDelta = 
			MSEC2TCKS(spx2OptReq->spxIIMaximumRetryDelta);
		if (spx2OptReq->spxIIMaximumRetryDelta < minDeltaTicks ) 
			spx2OptReq->spxIIMaximumRetryDelta = minDeltaTicks;
		else if (spx2OptReq->spxIIMaximumRetryDelta > maxDeltaTicks ) 
			spx2OptReq->spxIIMaximumRetryDelta = maxDeltaTicks;

		conEntryPtr->maxTicksToWait = spx2OptReq->spxIIMaximumRetryDelta;
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TSetOptmgmt: Set Max Retry Delta to %d ticks",
				conEntryPtr->maxTicksToWait));

		spx2OptReq->spxIIMaximumRetryDelta = 
			TCKS2MSEC(spx2OptReq->spxIIMaximumRetryDelta);

		if (spx2OptReq->spxIILocalWindowSize == 0 ) 
			spx2OptReq->spxIILocalWindowSize = SPX2_WINDOW_SIZE;
		else if (spx2OptReq->spxIILocalWindowSize < MIN_SPX2_WINDOW_SIZE) 
			spx2OptReq->spxIILocalWindowSize = MIN_SPX2_WINDOW_SIZE;
		else if (spx2OptReq->spxIILocalWindowSize > MAX_SPX2_WINDOW_SIZE) 
			spx2OptReq->spxIILocalWindowSize = MAX_SPX2_WINDOW_SIZE;

		conEntryPtr->spxWinSize = spx2OptReq->spxIILocalWindowSize;
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TSetOptmgmt: Set Window Size to %d",
				spx2OptReq->spxIILocalWindowSize));
	} else {
		spxOptReq = optReq;
		if (spxOptReq->spxo_retry_count < MIN_SPX_TRETRIES_COUNT) 
			spxOptReq->spxo_retry_count = MIN_SPX_TRETRIES_COUNT;
		else if (spxOptReq->spxo_retry_count > MAX_SPX_TRETRIES_COUNT)
			spxOptReq->spxo_retry_count = MAX_SPX_TRETRIES_COUNT;

		conEntryPtr->tMaxRetries = spxOptReq->spxo_retry_count;
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TSetOptmgmt: Set Retry Count to %d",
				spxOptReq->spxo_retry_count));

		if (spxOptReq->spxo_min_retry_delay < 
			(uint16)MSEC2TCKS(MIN_SPX2_MIN_RETRY_DELAY))
			spxOptReq->spxo_min_retry_delay = 
					(uint16)MSEC2TCKS(MIN_SPX2_MIN_RETRY_DELAY);
		else if (spxOptReq->spxo_min_retry_delay > 
					(uint16)MSEC2TCKS(MAX_SPX2_MIN_RETRY_DELAY))
			spxOptReq->spxo_min_retry_delay = 
					(uint16)MSEC2TCKS(MAX_SPX2_MIN_RETRY_DELAY);

		conEntryPtr->minTicksToWait = spxOptReq->spxo_min_retry_delay;
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TSetOptmgmt: Set Min Retry Delay to %d ticks",
				spxOptReq->spxo_min_retry_delay));
	}
	Exit:
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX:TOptmgmt: TSetOptmgmt EXIT"));
		return(NTR_LEAVE(rtval));
}

/*
 * void SpxTConnReq(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxTConnReq(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;
	ipxMctl_t *spxMctlPtr;	
	struct T_conn_req *tConnReq;
	uint16 conId;
	mblk_t *nmp;
	void *optReq;
	uint32 netwrk;
	long tConnReqError;
	long tConnReqUnixError;
	spxUnitHdr_t *desAddr;
	int FreeIt = 1;
	
	NTR_ENTER( 2, q, mp, 0, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *) q->q_ptr;
	conId = conEntryPtr->spxHdr.sourceConnectionId;
	tConnReqError = 0;
	tConnReqUnixError = 0;

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: uws: ENTER TConnReq"));

	if (DATA_SIZE(mp) < (sizeof(struct T_conn_req) + sizeof(ipxAddr_t))){
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnReq: TLI + ipxAddr bad size %d",DATA_SIZE(mp)));
		tConnReqError = TBADADDR;
		spxStats.spx_connect_req_fails++;
		goto ErrorAck;
	}

	if (conEntryPtr->state != TS_IDLE) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnReq: endpoint in invalid state %d",
			conEntryPtr->state));
		tConnReqError = TOUTSTATE;
		spxStats.spx_connect_req_fails++;
		goto ErrorAck;
	}

	if (mp->b_cont) {
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: TConnReq: NO data allowed on Connect Request"));
		tConnReqError = TBADDATA;
		goto ErrorAck;
	}

	conEntryPtr->state = TS_WACK_CREQ;

	tConnReq = (struct T_conn_req *) mp->b_rptr;
	desAddr  = (spxUnitHdr_t *) (mp->b_rptr + tConnReq->DEST_offset);

	if ((conEntryPtr->spx2Options) && 
		(tConnReq->OPT_length >= VERSION1SPX2OPTSIZE)) {
		
		optReq = (void *)(mp->b_rptr + tConnReq->OPT_offset);
		SpxTSetOptmgmt(conEntryPtr, optReq, TRUE);
	}

	FreeIt = 0;
	freemsg(mp);
		
	/*
	** set the connection id to a new value.  ie. we keep the lower 10 bits
	** the same, as these are the index into the spxConnectionTable.
	** we increment the value of the upper 6 bits, allowing a wrap if it
	** occurs.
	*/
	conId = ((((conId >> 10) + 1) << 10) & 0xFC00) + (conId & CONIDTOINDEX);
	conEntryPtr->spxHdr.sourceConnectionId = conId;

/* set up packet info for connection request */
	conEntryPtr->spxHdr.ipxHdr.dest = desAddr->addr;
	conEntryPtr->spxHdr.connectionControl = 
		(SPX_SYSTEM_PACKET | SPX_ACK_REQUESTED );
	conEntryPtr->spxHdr.connectionControl |= conEntryPtr->protocol; 
	conEntryPtr->spxHdr.destinationConnectionId = SPX_CON_REQUEST_CONID;

/* send M_CTL to ipx to get the drivers maxium packet size.  When
 * IPX returns the M_CTL processing will continue at SpxTConnReqCont.
 */
	if ((nmp = allocb(sizeof(ipxMctl_t), BPRI_HI)) == NULL) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnReq: cant alloc %d ",
			sizeof(ipxMctl_t)));
		goto ErrorAck;
	}

	IPXCOPYNET(conEntryPtr->spxHdr.ipxHdr.dest.net, &netwrk);

	MTYPE(nmp) = M_CTL;
	nmp->b_wptr = nmp->b_rptr + sizeof(ipxMctl_t); 
	spxMctlPtr = (ipxMctl_t *)nmp->b_rptr;	
	spxMctlPtr->mctlblk.cmd = SPX_GET_IPX_MAX_SDU; 
	spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.network = netwrk; 
	spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.maxSDU = 0; 
	spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.conEntry 
			= (caddr_t)conEntryPtr; 

/* this is a priority message send it regardless of canput */
	 putnext(spxbot,nmp);

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: uws: EXIT TConnReq"));

	NTR_VLEAVE();
	return;

	ErrorAck:
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnReq: errorAck t_errno %d errno %d",
			tConnReqError, tConnReqUnixError));

		if (conEntryPtr->state == TS_WACK_CREQ)
			conEntryPtr->state = TS_IDLE;
		if (FreeIt)
			freemsg(mp);

		SpxGenTErrorAck(q, (long) T_CONN_REQ, tConnReqError,tConnReqUnixError); 

		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: uws: TConnReq Error EXIT"));
		NTR_VLEAVE();
		return;
}

/*
 * void SpxTConnReqCont(spxConnectionEntry_t *conEntryPtr, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxTConnReqCont(spxConnectionEntry_t *conEntryPtr, mblk_t *mp)
{
	mblk_t *nmp = NULL;
	ipxMctl_t *spxMctlPtr;	
	struct T_ok_ack *tOkAck;
	long tConnReqError;
	uint32 ipxMaxSDU;
	long tConnReqUnixError;

	NTR_ENTER( 2, conEntryPtr, mp, 0, 0, 0);
	if (conEntryPtr == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: SpxTConnConCont called with NULL ptr")); 
		freemsg(mp);
		NTR_VLEAVE();
		return;
	}
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: ENTER TConnReqCont")); 

	tConnReqError = 0;
	tConnReqUnixError = 0;
	spxMctlPtr = (ipxMctl_t *)mp->b_rptr;	

	if (conEntryPtr->protocol == (SPX_SPXII | SPX_NEG)) {
		ipxMaxSDU = spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.maxSDU;
		if (ipxMaxSDU < SPX_DEFAULT_PACKET_SIZE) 
			conEntryPtr->maxSDU = SPX_DEFAULT_PACKET_SIZE;
		else 
			if (ipxMaxSDU > SPX_ABSOLUTE_MAX_PACKET_SIZE) 
				conEntryPtr->maxSDU = SPX_ABSOLUTE_MAX_PACKET_SIZE;
		else 
			conEntryPtr->maxSDU = ipxMaxSDU;
	} else {
		conEntryPtr->maxSDU = SPX_DEFAULT_PACKET_SIZE;
		conEntryPtr->sessionFlags = NO_NEGOTIATE;
		/* if No negotiate, can not have checksums.*/
		conEntryPtr->ipxChecksum = FALSE;
	}

	conEntryPtr->maxTPacketSize = 
	conEntryPtr->maxRPacketSize = conEntryPtr->maxSDU;
	conEntryPtr->window.local.alloc = conEntryPtr->spxWinSize -1;
	freemsg(mp);

	if ((nmp = allocb(sizeof(struct T_ok_ack),BPRI_MED)) == NULL ) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: TConnReqCont: cant alloc %d",
			sizeof(struct T_ok_ack))); 
		tConnReqError = TSYSERR;
		tConnReqUnixError = ENOSR;
		goto ErrorAck;
	}

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX:TConnReqCont: Send Connect Request to 0x%X%X%X",
		PGETINT32(&conEntryPtr->spxHdr.ipxHdr.dest.net),
		PGETINT32(&conEntryPtr->spxHdr.ipxHdr.dest.node),
		PGETINT32(&conEntryPtr->spxHdr.ipxHdr.dest.node[4])));

	if ((tConnReqUnixError = 
			SpxTranData(conEntryPtr, NULL, (uint8)TRUE, (uint8)FALSE)) != 0)  {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnReqCont: SpxTranData failed ")); 
		tConnReqError = TSYSERR;
		spxStats.spx_connect_req_fails++;
		goto ErrorAck;
	}

	conEntryPtr->state = TS_WCON_CREQ;

	tOkAck = (struct T_ok_ack *)nmp->b_rptr;
	tOkAck->PRIM_type = T_OK_ACK;
	tOkAck->CORRECT_prim = T_CONN_REQ;
	MTYPE(nmp) = M_PCPROTO;
	nmp->b_wptr = nmp->b_rptr + sizeof(struct T_ok_ack);
	putnext(conEntryPtr->upperReadQueue, nmp);

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: uws: EXIT TConnReqCont")); 

	NTR_VLEAVE();
	return;

	ErrorAck:
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnReqCont: errorAck t_errno %d errno %d",
			tConnReqError, tConnReqUnixError)); 

		if (conEntryPtr->state == TS_WACK_CREQ)
			conEntryPtr->state = TS_IDLE;
		if (nmp)
			freemsg(nmp);

		SpxGenTErrorAck(conEntryPtr->upperReadQueue, 
			(long) T_CONN_REQ, tConnReqError, tConnReqUnixError); 

		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: uws: TConnReqCont Error EXIT")); 
		NTR_VLEAVE();
		return;
}

/*
 * void SpxTConnRes(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void 
SpxTConnRes(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;
	register unsigned long seqNumber;
	struct T_conn_res *tConnRes;
	struct T_ok_ack *tOkAck;
	void *optReq;
	long tError, tUnixError =0;
	queue_t *resQueue;
	uint16 thisStreamSocket, acceptSock;
	uint16 localProtocol;
	int socketIndex, i, index , notAckedCount;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	/*
	 * t_accept (T_CONN_RES) comes down on the listening queue.
	 * thisStreamSocket will be set to the listening socket.
	 */
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;
	IPXCOPYSOCK(conEntryPtr->spxHdr.ipxHdr.src.sock, &thisStreamSocket);

	if (DATA_SIZE(mp) < sizeof(struct T_conn_res)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnRes: bad TLI header size %d",DATA_SIZE(mp)));
		tError = TSYSERR;
		tUnixError = EBADMSG;
		goto ErrorAck;
	}

	tConnRes = (struct T_conn_res *)mp->b_rptr;
	seqNumber = tConnRes->SEQ_number;
	resQueue = tConnRes->QUEUE_ptr;
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: uws: TConnRes: ENTER %d",seqNumber));

	if (conEntryPtr->state != TS_WRES_CIND) { 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnRes: listening q %Xh in bad state %d",
			q, conEntryPtr->state)); 
		tError = TOUTSTATE;
		goto ErrorAck;
	}

	for (socketIndex=0; socketIndex<spxSocketCount; socketIndex++)
		if(spxSocketTable[socketIndex].socketNumber == thisStreamSocket)
			break;

	if (socketIndex >= spxSocketCount) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnRes: socket N %Xh not in socket table",
			thisStreamSocket)); 
		tError = TSYSERR;
		tUnixError = ENXIO;
		goto ErrorAck;
	}

	if (seqNumber >= spxMaxListensPerSocket) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnRes: invalid sequence num %d",
			seqNumber));
		tError = TBADSEQ;
		goto ErrorAck;
	}

	if (spxSocketTable[socketIndex].listens[seqNumber].acked) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnRes sequence %d already acked",
			seqNumber));
		tError = TBADSEQ;
		goto ErrorAck;
	}

	/* 
		weather this ack fails or succeeds this connection request is
		marked as acked.  prevents conn reqs hanging around.
	*/

	spxSocketTable[socketIndex].listens[seqNumber].acked = TRUE;

	/*
		if we are accepting on our own queue then there must be 
		no outstanding connection requests on this queue.
		if this is the last con req to be acked and its not 
		accepting on its own queue then the state 
		goes to TS_IDLE else goes to TS_data_xfer
	*/

	notAckedCount = 0;
	for (i=0; i<spxMaxListensPerSocket; i++) {
		if (!spxSocketTable[socketIndex].listens[i].acked)
			notAckedCount++;
	}
	
	/* is this the last connection request ack? if so change state */
	if (notAckedCount == 0)
		conEntryPtr->state = TS_IDLE;

	for (i=0; i<spxMaxConnections; i++) 
		if (resQueue == spxConnectionTable[i].upperReadQueue)
			break;

	if (i >= spxMaxConnections) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnRes accepting q %Xh invalid",resQueue));
		/* set listening connection back to original state */
		conEntryPtr->state = TS_WRES_CIND;
		spxSocketTable[socketIndex].listens[seqNumber].acked = FALSE;
		tError = TSYSERR;
		tUnixError = ENODEV;
		goto ErrorAck;
	}

	if (spxConnectionTable[i].state != TS_IDLE) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnRes accepting q %Xh invalid state %d",
			resQueue, spxConnectionTable[i].state));
		/* set listening connection back to original state */
		conEntryPtr->state = TS_WRES_CIND;
		spxSocketTable[socketIndex].listens[seqNumber].acked = FALSE;
		tError = TOUTSTATE;
		goto ErrorAck;
	}

	/*
	 *	if we are not accepting the connection on our own queue then
	 *	the accepting queue must be bound with qlen of zero.
	 *	(i.e. it is not another listener ...)
	 *  Only sockets bound with qlen > 0 is in the spxSocketTable.
	 *	Test to ensure the accpecting socket is NOT in the spxSocketTable
	 */
	

	IPXCOPYSOCK(spxConnectionTable[i].spxHdr.ipxHdr.src.sock, &acceptSock);
	NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
		"TConnRes: accepting sock 0x%x, listening sock0x%x",
		GETINT16(acceptSock), GETINT16(thisStreamSocket)));
	NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
		"TConnRes: listening que 0x%x, accpecting que 0x%x",
			q, OTHERQ(resQueue)));

	if (q != OTHERQ(resQueue)) {
		for (index=0; index < spxSocketCount; index++) {
			if (spxSocketTable[index].socketNumber == acceptSock) {
				break;
			}
		}
		/* if we found it, it was bound as a listener. */
		if (index < spxSocketCount) { 
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uws: TConnRes when accepting q %Xh and our q %Xh are not the same,\naccepting q should not be a listener.  Its qlen should be zero. Its value is %d"
				, OTHERQ(resQueue), q, spxSocketTable[index].postedListens));
			/* set listening connection back to original state */
			conEntryPtr->state = TS_WRES_CIND;
			spxSocketTable[socketIndex].listens[seqNumber].acked = FALSE;
			tError = TRESQLEN;
			goto ErrorAck;
		}
	}

/* set up connection table (same time packet) and send ack */
	
	spxConnectionTable[i].spxHdr.ipxHdr.dest = 
		spxSocketTable[socketIndex].listens[seqNumber].spxInfo.addr;
	spxConnectionTable[i].spxHdr.dataStreamType = 0;
	spxConnectionTable[i].spxHdr.destinationConnectionId = 
		spxSocketTable[socketIndex].listens[seqNumber].
		spxInfo.connectionId;
	spxConnectionTable[i].spxHdr.sequenceNumber =0;
	spxConnectionTable[i].spxHdr.acknowledgeNumber =0;
	spxConnectionTable[i].window.local.sequence = 0;
	spxConnectionTable[i].window.local.ack = 0;
	spxConnectionTable[i].window.local.alloc = 0;

	spxConnectionTable[i].window.remote.alloc =
		spxSocketTable[socketIndex].listens[seqNumber].spxInfo.allocationNumber;
	spxConnectionTable[i].responseQueue = q; /* save q to send t_ok_ack on */ 
	IPXCOPYSOCK(&thisStreamSocket, &spxConnectionTable[i].listenSocket);
	IPXCOPYSOCK(&spxConnectionTable[i].spxHdr.ipxHdr.src.sock,
						&spxConnectionTable[i].acceptSocket);
	if (conEntryPtr->protocol & SPX_SPXII ) {
		IPXCOPYSOCK(&spxConnectionTable[i].listenSocket,
			&spxConnectionTable[i].spxHdr.ipxHdr.src.sock);
	}

	if (!(conEntryPtr == &spxConnectionTable[i])) {
		spxConnectionTable[i].protocol = conEntryPtr->protocol;
		spxConnectionTable[i].disabled = conEntryPtr->disabled;
		spxConnectionTable[i].sessionFlags = conEntryPtr->sessionFlags;
		spxConnectionTable[i].maxSDU = conEntryPtr->maxSDU;
		spxConnectionTable[i].maxRPacketSize = 
		spxConnectionTable[i].maxTPacketSize = conEntryPtr->maxTPacketSize;
		spxConnectionTable[i].ticksToNet = conEntryPtr->ticksToNet;
		spxConnectionTable[i].spxWinSize = conEntryPtr->spxWinSize;
		spxConnectionTable[i].ipxChecksum = conEntryPtr->ipxChecksum;

		conEntryPtr->sessionFlags = 0;
		conEntryPtr->maxSDU =
		conEntryPtr->maxTPacketSize =
		conEntryPtr->maxRPacketSize = SPX_DEFAULT_PACKET_SIZE;
		conEntryPtr->protocol = ( SPX_SPXII | SPX_NEG );
		conEntryPtr->ipxChecksum = spxIpxChecksum;
		bzero((char *)&conEntryPtr->spxHdr.ipxHdr.dest, sizeof(ipxAddr_t));
	}

	if ((spxConnectionTable[i].spx2Options) && 
		(tConnRes->OPT_length >= VERSION1SPX2OPTSIZE)) {
		
		optReq = (void *)(mp->b_rptr + tConnRes->OPT_offset);
		SpxTSetOptmgmt(&spxConnectionTable[i], optReq, FALSE);
	}
		
	spxConnectionTable[i].window.local.alloc = 
						spxConnectionTable[i].spxWinSize -1;

	/* Save protol byte locally, will need to test the original value
	 * after SendAck.  If the other endpoint is on the same box, could
	 * receive Negotiate Request packet from other endpoint before
	 * this function gets control back.  Set waiting for NegReq flag 
	 * before we send the ack.  Test the Original protocol value 
	 * (it might have changed in connectionTable) to determine if 
	 * we should send the SessionSetup packet.
	 */
	localProtocol = spxConnectionTable[i].protocol;
	if (spxConnectionTable[i].protocol & SPX_SPXII) {
		if (spxConnectionTable[i].protocol & SPX_NEG) {
			spxConnectionTable[i].sessionFlags |= WAIT_NEGOTIATE_REQ;

			/* setup negotiation TIMEOUT, if no Negotiation Req within time 
			   period disconnect
			*/
			spxConnectionTable[i].nTimeOutId = itimeout(SpxNegTimeOut,
				(void *)i, spxWatchEmuInterval, pltimeout);
		}
	}
			
	if (tUnixError = SpxSendAck(i, FALSE, 0)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnRes SendAck failed %d",tUnixError));
		/* set listening connection back to original state */
		conEntryPtr->state = TS_WRES_CIND;
		spxSocketTable[socketIndex].listens[seqNumber].acked = FALSE;
		/*
		 * Reset destination fields of accepting endpoint's connection
		 * table entry to indicate that connection has not been
		 * established.
		 */
		bzero((caddr_t)(&spxConnectionTable[i].spxHdr.ipxHdr.dest),
		 sizeof(ipxAddr_t));
		spxConnectionTable[i].spxHdr.destinationConnectionId = (uint16)0;
		spxConnectionTable[i].responseQueue = NULL; 
		tError = TSYSERR;
		goto ErrorAck;
	}
	/* 
	 * If SPXII and we ARE negotiating size, wait for Negotiate Size 
	 * packet from the client. We just sent the connection Ack.
	 * We will send the T_CONN_RES ack after size negotiation is done.
	 *
	 * If SPXII and we are NOT negotiating size, we need to send the
	 * SESSION_SETUP packet now. Set the SESS_SETUP bit in the session
	 * flags and call SpxSendNegSessPkt. SpxSendNegSessPkt will fill
	 * in the information needed. We will send the T_CONN_RES ack after
	 * we recieve the Session Setup Ack.
	 * 
	 * If NOT SPXII, set to TS_DATA_XFER state and ack the T_CONN_RES.
	 * We just sent the connection Ack and do not need to send any more
	 * packets to the client.
	 */
	if (spxConnectionTable[i].protocol & SPX_SPXII) {
		if (!(localProtocol & SPX_NEG)) {
			spxConnectionTable[i].sessionFlags |= SESS_SETUP;
			spxConnectionTable[i].sizeNegoRetry = 0;
			SpxSendNegSessPkt(&spxConnectionTable[i]);
		}
		freemsg(mp);
	} else {
		spxConnectionTable[i].state = TS_DATA_XFER;
		spxConnectionTable[i].responseQueue = NULL; 
		MTYPE(mp) = M_PCPROTO;
		tOkAck = (struct T_ok_ack *)mp->b_rptr;
		tOkAck->PRIM_type = T_OK_ACK;
		tOkAck->CORRECT_prim = T_CONN_RES;
		mp->b_wptr = mp->b_rptr+sizeof(struct T_ok_ack);
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = 0;
		}
		qreply(q,mp);
	}
	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: uws: TConnRes EXIT"));

	NTR_VLEAVE();
	return;

	ErrorAck:
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TConnRes: ErrorAck t_errno %d errno %d",
			tError,tUnixError));

		freemsg(mp);
		SpxGenTErrorAck(q, (long) T_CONN_RES, tError, tUnixError);

		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: uws: TConnRes: Error EXIT"));
		NTR_VLEAVE();
		return;
}

/*
 * void SpxTOrdRelReq(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxTOrdRelReq(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *) q->q_ptr;

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: uws: TOrdRelReq: ENTER "));
	/*	If the connection is not SPXII return EPROTO, otherwise 
	**  the connection could hang waiting for Orderly release from 
	**  other endpoint.
	*/
	if (!(conEntryPtr->protocol & SPX_SPXII)) {
		NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
		"NSPX: uws: T_ORDREL_REQ: Not SPXII Connection"));
		goto Error;
	}

	if ((conEntryPtr->state != TS_DATA_XFER) &&
			(conEntryPtr->state != TS_WREQ_ORDREL)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: T_ORDREL_REQ bad state"));
		goto Error;
	}

	conEntryPtr->spxHdr.dataStreamType = SPX_ORDERLY_REL_REQ;
	conEntryPtr->spxHdr.connectionControl = (SPX_EOF_BIT | SPX_ACK_REQUESTED);
	conEntryPtr->spxHdr.connectionControl |= conEntryPtr->protocol; 

	conEntryPtr->ordRelReqSent = 1;
	conEntryPtr->ordSeqNumber = conEntryPtr->window.local.sequence;
	if (SpxTranData(conEntryPtr, NULL, (uint8)TRUE, (uint8)TRUE)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TOrdRelReq: TranData failed "));
		putbq(q,mp);
		conEntryPtr->ordRelReqSent = 0;
		conEntryPtr->ordSeqNumber = 0;
		conEntryPtr->spxHdr.dataStreamType = 0;
		NTR_VLEAVE();
		return;
	}
	conEntryPtr->spxHdr.dataStreamType = 0;
	freemsg(mp);
	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: uws: EXIT TOrdRelReq"));
	NTR_VLEAVE();
	return;

Error:
	MTYPE(mp) = M_ERROR;
	mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
	*(mp->b_rptr) = EPROTO;
	mp->b_wptr++;
	qreply(q,mp);
	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: uws: ERROR EXIT TOrdRelReq sent M_ERROR"));
	NTR_VLEAVE();
}

/*
 * void SpxTDisReq(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxTDisReq(queue_t *q, mblk_t *mp)
{
	register spxConnectionEntry_t *conEntryPtr;
	mblk_t *flushMp;
	uint16 conId, conSocket;
	long tError;
	long tUnixError;
	int i,j;
	struct T_discon_req *tDisConReq;
	struct T_ok_ack *tOkAck;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;
	GETALIGN16(&conEntryPtr->spxHdr.sourceConnectionId,&conId);
	conId &= CONIDTOINDEX;

	if (DATA_SIZE(mp) < sizeof(struct T_discon_req)) {
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: uws: TDisReq: bad TLI header size %d",DATA_SIZE(mp)));
		tError = TSYSERR;
		tUnixError = EBADMSG;
		goto ErrorAck;
	}

	if (mp->b_cont) {
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: uws: TDisReq: NO data allowed on Disconnect"));
		tError = TBADDATA;
		goto ErrorAck;
	}
		
	tDisConReq = (struct T_discon_req *)mp->b_rptr;

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"NSPX: uws: ENTER TDisReq:"));

	if ((tDisConReq->SEQ_number != -1) && 
		 (conEntryPtr->state == TS_WRES_CIND)) {

		if (tDisConReq->SEQ_number >= spxMaxListensPerSocket) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uws: TDisReq: invalid sequence num %d",
				tDisConReq->SEQ_number));
			tError = TBADSEQ;
			goto ErrorAck;
		}


		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: uws: TDisReq: Denying sequence %d",
				tDisConReq->SEQ_number));

		IPXCOPYSOCK(conEntryPtr->spxHdr.ipxHdr.src.sock,&conSocket);

		for (i=0; i< spxSocketCount; i++)
			if (conSocket == spxSocketTable[i].socketNumber)
				break;

		if (i >= spxSocketCount) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uws: TDisReq: Socket Not found"));
			tError = TSYSERR;
			tUnixError = ENXIO;
			goto ErrorAck;
		}

		if (spxSocketTable[i].listens[tDisConReq->SEQ_number].acked) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uws: TDisReq: listen %d already acked",
				tDisConReq->SEQ_number));
			tError = TBADSEQ;
			goto ErrorAck;
		}

	/* Send acknowledge to Connect request the disconnect Immediatly, Native
	 * SPX does it this way, this eliminates retry/timeout on connect
	 * Requests when the application rejects it.
	 */ 

		/* Set up connection Table first temporarily */
		conEntryPtr->spxHdr.ipxHdr.dest = 
			spxSocketTable[i].listens[tDisConReq->SEQ_number].spxInfo.addr;
		conEntryPtr->spxHdr.dataStreamType = 0;
		conEntryPtr->spxHdr.destinationConnectionId = 
			spxSocketTable[i].listens[tDisConReq->SEQ_number].
														spxInfo.connectionId;
		conEntryPtr->spxHdr.sequenceNumber =0;
		conEntryPtr->spxHdr.acknowledgeNumber =0;
		conEntryPtr->window.local.sequence = 0;
		conEntryPtr->window.local.ack = 0;
		conEntryPtr->window.local.alloc = 0xffff;	/* close window */
		conEntryPtr->protocol = SPX_SPXII;

		SpxSendAck(conId, FALSE, 0);
		SpxSendDisconnect(conEntryPtr, (uint8) FALSE); /* do not retry */

		bzero((char *)&conEntryPtr->spxHdr.ipxHdr.dest,sizeof(ipxAddr_t));
		conEntryPtr->protocol = (SPX_SPXII | SPX_NEG);
		conEntryPtr->spxHdr.destinationConnectionId = 0;
		conEntryPtr->window.local.alloc = 0;

		/* End of Change */

		spxSocketTable[i].listens[tDisConReq->SEQ_number].acked = TRUE;

		for(j=0; j<spxMaxListensPerSocket; j++)
			if (!spxSocketTable[i].listens[j].acked)
				break;
		
		if (j>=spxMaxListensPerSocket) 
			conEntryPtr->state = TS_IDLE;

		NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE,
			"NSPX:TDisReq: Connection refused state= %d",
			conEntryPtr->state));

		goto PositiveAck;
	}
	if ((conEntryPtr->state != TS_DATA_XFER) &&
		(conEntryPtr->state != TS_WREQ_ORDREL) &&
		(conEntryPtr->state != TS_WIND_ORDREL)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TDisReq: invalid state for tdisconreq %d",
			conEntryPtr->state));
		tError = TOUTSTATE;
		tUnixError = 0;
		goto ErrorAck;
	}

/* set up outbound packet */
	SpxSendDisconnect(conEntryPtr, (uint8) TRUE); /* retry */
	conEntryPtr->sessionFlags |= WAIT_TERMINATE_ACK;
	freemsg(mp);

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: uws: TDisReq: EXIT "));

	NTR_VLEAVE();
	return;


	PositiveAck:

/* per tli spec we must flush before putting up */
		if ((flushMp = allocb(1, BPRI_MED)) == NULL ) {
			spxStats.spx_alloc_failures++; 
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
				"NSPX: uws: TDisReq cant alloc msg size 1"));
		} else {
			*(flushMp->b_rptr) = FLUSHR | FLUSHW; /* flushFlags */
			MTYPE(flushMp) = M_FLUSH;
			flushMp->b_wptr = flushMp->b_rptr + 1;
			qreply(q,flushMp);
		}

		tOkAck = (struct T_ok_ack *)mp->b_rptr;
		tOkAck->PRIM_type = T_OK_ACK;
		tOkAck->CORRECT_prim = T_DISCON_REQ;
		mp->b_wptr = mp->b_rptr + sizeof(struct T_ok_ack);
		MTYPE(mp) = M_PCPROTO;
		if (mp->b_cont) {
			freemsg(mp->b_cont);
			mp->b_cont = 0;
		}
		qreply(q,mp);

		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: uws: TDisReq: EXIT "));

		NTR_VLEAVE();
		return;

	ErrorAck:
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: lrp: TDisReq: ErrorAck t_errno %d errno %d",
			tError, tUnixError));

		freemsg(mp);
		SpxGenTErrorAck(q, (long) T_DISCON_REQ, tError, tUnixError);

		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: uws: TDisReq: EXIT Error "));
		NTR_VLEAVE();
		return;
} 


/*
 * void SpxTUnbindReq(queue_t *q, uint8 needTliAck)
 *	Unbind a IPX socket that is bound.
 *	Since this routine could be called at any time from spxclose
 *	we have to account for the fact that an ack may be needed or
 *	may not be needed.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void 
SpxTUnbindReq(queue_t *q, uint8 needTliAck)
{
	register spxConnectionEntry_t *conEntryPtr;
	int 	i,j;
	mblk_t *okAckMp, *flushMp;
	uint16 conId;
	struct T_ok_ack *tOkAck;
	long tError, tUnixError;
	uint16 socketToUnbind;
	queue_t		*wrQueue;

	NTR_ENTER( 2, q, needTliAck, 0, 0, 0);
	conEntryPtr=(spxConnectionEntry_t *)q->q_ptr;
	conId = conEntryPtr->spxHdr.sourceConnectionId;

	IPXCOPYSOCK(conEntryPtr->spxHdr.ipxHdr.src.sock, &socketToUnbind);

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE, 
		   "NSPX: TUnbind: ENTER unbinding  socket N %Xh",
			GETINT16(socketToUnbind)));

	tError = 0;
	tUnixError = 0;

	if (conEntryPtr->state != TS_IDLE) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TUnBind: bad state %d",conEntryPtr->state));
		tError = TOUTSTATE;
		tUnixError = 0;
		goto ErrorAck;
	}

	for (i=0; i< spxSocketCount; i++) 
		if (conId == spxSocketTable[i].listenConnectionId)
			break;

	if (i < spxSocketCount) { 
		NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE,
		"NSPX: TUnBind: Listening ConnId %d found, clearing",conId));
		/*
		 * If we are a listener, check and see if our write queue
		 * is being used for connection responses by any other device.
		 */ 
		wrQueue = WR(q);
		for (j=0; j < spxSocketCount; j++) {
			if (wrQueue == spxConnectionTable[j].responseQueue) {
				NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE,
					"NSPX: TUnBind: NULL responseQueue for conID %d",
					spxConnectionTable[j].spxHdr.sourceConnectionId));
				spxConnectionTable[j].responseQueue = NULL;
			}
		}
		bzero((char *)&spxSocketTable[i],sizeof(spxSocketEntry_t));
		for (j=0; j<spxMaxListensPerSocket; j++)
			spxSocketTable[i].listens[j].acked = TRUE;
	} 
#ifdef DEBUG
	 else {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: TUnbind: id %d not found",conId));
	}
#endif
	/*
	** Release IPX_SOCKET
	*/ 
	SpxRelIpxSocket(GETINT16(socketToUnbind));

	socketToUnbind = 0;
	IPXCOPYSOCK(&socketToUnbind, conEntryPtr->spxHdr.ipxHdr.src.sock);
	conEntryPtr->state = TS_UNBND;

/*	PositiveAck: */
	if (!needTliAck) goto Exit;


	if ((flushMp = allocb(1, BPRI_MED)) == NULL ) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: TUnbind: cant alloc flush size 1"));
		tError = TSYSERR;
		tUnixError = ENOSR;
		goto ErrorAck;
	}

	*(flushMp->b_rptr) = FLUSHR  | FLUSHW; /* flushFlags */
	MTYPE(flushMp) = M_FLUSH;
	flushMp->b_wptr = flushMp->b_rptr + 1;
	qreply(q,flushMp);

	if ((okAckMp = allocb(sizeof(struct T_ok_ack),BPRI_MED)) == NULL ) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: TUnbind: cant alloc %d",
			sizeof(struct T_ok_ack)));
		tError = TSYSERR;
		tUnixError = ENOSR;
		goto ErrorAck;
	}
	tOkAck = (struct T_ok_ack *) okAckMp->b_rptr;
	tOkAck->PRIM_type = T_OK_ACK;
	tOkAck->CORRECT_prim = T_UNBIND_REQ;
	MTYPE(okAckMp) = M_PCPROTO;
	okAckMp->b_wptr = okAckMp->b_rptr + sizeof(struct T_ok_ack);
	qreply(q,okAckMp);

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, 
	   "NSPX: TUnbind: EXIT"));

	NTR_VLEAVE();
	return;

	ErrorAck:
		if (!needTliAck) goto Exit;

		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
		   "NSPX: TUnbind: Error t_errno %d errno %d",
		   tError, tUnixError));

		SpxGenTErrorAck(q, (long) T_UNBIND_REQ, tError, tUnixError);

	Exit:
		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, 
		   "NSPX: TUnbind:  EXIT procedure"));
		   
		NTR_VLEAVE();
		return;
}

/*
 * long SpxTCheckOptmgmt(spxConnectionEntry_t *conEntryPtr, void *optReq)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
long
SpxTCheckOptmgmt(spxConnectionEntry_t *conEntryPtr, void *optReq)
{
	SPX2_OPTIONS *spx2OptReq;
	SPX_OPTMGMT *spxOptReq;

	NTR_ENTER( 3, conEntryPtr, optReq, 0, 0, 0);
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: ENTER TCheckOptmgmt"));

	if (conEntryPtr->spx2Options) {
		spx2OptReq = optReq;
		if ((spx2OptReq->versionNumber == 0 ) ||
			(spx2OptReq->versionNumber > OPTIONS_VERSION)) {
			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TCheckOptmgmt:EXIT T_FAILURE, Invalid versionNumber %d",
				spx2OptReq->versionNumber));
			return(NTR_LEAVE(T_FAILURE));
		}

		if (spx2OptReq->spxIISessionFlags & ~(SPX_SF_IPX_CHECKSUM)) {
			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			  "NSPX:TCheckOptmgmt:EXIT T_FAILURE Invalid spxIISessionFlags %d",
				spx2OptReq->spxIISessionFlags ));
			return(NTR_LEAVE(T_FAILURE));
		}

		if ((spx2OptReq->spxIIOptionNegotiate != SPX2_NEGOTIATE_OPTIONS) &&
			(spx2OptReq->spxIIOptionNegotiate != SPX2_NO_NEGOTIATE_OPTIONS)) {

			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			  "NSPX:TCheckOptmgmt:EXIT T_FAILURE, Invalid optionNegotiate %d",
				spx2OptReq->spxIIOptionNegotiate));
			return(NTR_LEAVE(T_FAILURE));
		}

		if ((spx2OptReq->spxIIRetryCount < MIN_SPX_TRETRIES_COUNT) ||
			(spx2OptReq->spxIIRetryCount > MAX_SPX_TRETRIES_COUNT)) {

			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TCheckOptmgmt:EXIT  T_FAILURE, Invalid RetryCount %d",
				spx2OptReq->spxIIRetryCount));
			return(NTR_LEAVE(T_FAILURE));
		}

		if ((spx2OptReq->spxIIMinimumRetryDelay < MIN_SPX2_MIN_RETRY_DELAY) ||
			(spx2OptReq->spxIIMinimumRetryDelay > MAX_SPX2_MIN_RETRY_DELAY)) {

			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TCheckOptmgmt:EXIT  T_FAILURE, Invalid RetryDelay %d",
				spx2OptReq->spxIIMinimumRetryDelay));
			return(NTR_LEAVE(T_FAILURE));
		}

		if ((spx2OptReq->spxIIMaximumRetryDelta < 
			TCKS2MSEC(MIN_SPX2_MAX_RETRY_DELTA * HZ )) ||
			(spx2OptReq->spxIIMaximumRetryDelta > 
			TCKS2MSEC(MAX_SPX2_MAX_RETRY_DELTA * HZ ))) {

			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			   "NSPX:TCheckOptmgmt:EXIT T_FAILURE, Invalid Max RetryDelta %d",
				spx2OptReq->spxIIMaximumRetryDelta));
			return(NTR_LEAVE(T_FAILURE));
		}

		if ((spx2OptReq->spxIILocalWindowSize < MIN_SPX2_WINDOW_SIZE) ||
			(spx2OptReq->spxIILocalWindowSize > MAX_SPX2_WINDOW_SIZE)) {

			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TCheckOptmgmt:EXIT  T_FAILURE, Invalid WindowSize %d",
				spx2OptReq->spxIILocalWindowSize));
			return(NTR_LEAVE(T_FAILURE));
		}
	} else {
		spxOptReq = optReq;
		if ((spxOptReq->spxo_retry_count < MIN_SPX_TRETRIES_COUNT) ||
			(spxOptReq->spxo_retry_count > MAX_SPX_TRETRIES_COUNT)) {

			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TCheckOptmgmt:EXIT  T_FAILURE, Invalid retry_count %d",
				spxOptReq->spxo_retry_count));
			return(NTR_LEAVE(T_FAILURE));
		}
			
		/* do not check spx_min_retry_delay, it was documented as a 
			unsupported option. */
		if ((spxOptReq->spxo_min_retry_delay < 
			(uint16)MSEC2TCKS(MIN_SPX2_MIN_RETRY_DELAY)) || 
			(spxOptReq->spxo_min_retry_delay > 
			(uint16)MSEC2TCKS(MAX_SPX2_MIN_RETRY_DELAY))) {

			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TCheckOptmgmt:EXIT  T_FAILURE, Invalid retry_delay %d",
				spxOptReq->spxo_min_retry_delay));
			return(NTR_LEAVE(T_FAILURE));
		}
		
		if (spxOptReq->spxo_watchdog_flag == 0) {

			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX:TCheckOptmgmt:EXIT T_FAILURE, Invalid watchdog_flag %d",
				spxOptReq->spxo_watchdog_flag));
			return(NTR_LEAVE(T_FAILURE));
		}
	}

	NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX:TOptmgmt: T_CHECK EXIT returning T_SUCCESS"));
	return(NTR_LEAVE(T_SUCCESS));
}

/*
 * void SpxTOptmgmt(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void 
SpxTOptmgmt(queue_t *q, mblk_t *mp)
{
	long tError, tUnixError;
	spxConnectionEntry_t *conEntryPtr;
	mblk_t *ackMp;
	int ackSize;
	long flags;
	long checkFlag = 0;
	struct T_optmgmt_req *tOptReq;
	struct T_optmgmt_ack *tOptAck;
	void *optReq;
	SPX2_OPTIONS *spx2OptReq;
	SPX_OPTMGMT *spxOptReq;
	SPX2_OPTIONS *spx2OptRet;
	SPX_OPTMGMT *spxOptRet;

	uint32	spx2negotiate, spx2retryCount, spx2sessionFlag;
	uint32  spx2retryDelay, spx2retryDelta, spx2windowSize;

	uint16	retryDelay;
	uint8 retryCount, watchDogFlag;

	NTR_ENTER( 2, q, mp, 0, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: uws: TOptMgmt: ENTER"));

	if (conEntryPtr->state != TS_IDLE) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TOptmgmt: bad state %d",conEntryPtr->state));
		tError = TOUTSTATE;
		tUnixError = 0;
		freemsg(mp);
		goto ErrorAck;
	}
		
	if (DATA_SIZE(mp) < sizeof(struct T_optmgmt_req)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TOptmgmt: req bad size %d",DATA_SIZE(mp)));
		tError= TBADOPT;
		tUnixError = 0;
		freemsg(mp);
		goto ErrorAck;
	}

	tOptReq = (struct T_optmgmt_req *)mp->b_rptr;
	flags = tOptReq->MGMT_flags;
	optReq = mp->b_rptr + tOptReq->OPT_offset;

	switch (flags) {
	case T_NEGOTIATE:
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: uws: TOptmgmt: doing T_NEGOTIATE"));

		if (tOptReq->OPT_length == 0) {
			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX: uws: TOptmgmt: OPT_length is 0"));
			goto spx_T_DEFAULT; 
		}

		if (tOptReq->OPT_length < sizeof(uint32)) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TOptmgmt: opts bad size %d",tOptReq->OPT_length));
			tError= TBADOPT;
			tUnixError = 0;
			freemsg(mp);
			goto ErrorAck;
		}
		if ((conEntryPtr->spx2Options) && 
			(tOptReq->OPT_length < VERSION1SPX2OPTSIZE)) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TOptmgmt: opts bad size %d",tOptReq->OPT_length));
			tError= TBADOPT;
			tUnixError = 0;
			freemsg(mp);
			goto ErrorAck;
		}

		if (!(SpxTSetOptmgmt(conEntryPtr, optReq, TRUE))) {
			if (conEntryPtr->spx2Options) {
				spx2OptReq = optReq;
				spx2negotiate = spx2OptReq->spxIIOptionNegotiate;
				spx2retryCount = spx2OptReq->spxIIRetryCount;
				spx2retryDelay = spx2OptReq->spxIIMinimumRetryDelay;
				spx2retryDelta = spx2OptReq->spxIIMaximumRetryDelta;
				spx2windowSize = spx2OptReq->spxIILocalWindowSize;
				spx2sessionFlag = spx2OptReq->spxIISessionFlags;
			} else {
				spxOptReq = optReq;
				retryCount = spxOptReq->spxo_retry_count;
				retryDelay = spxOptReq->spxo_min_retry_delay;
				watchDogFlag = 1;
			}
			goto PositiveAck;
		}
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
		"NSPX: uws: TOptmgmt: TSetOptmgmt Failed"));
		tError= TBADOPT;
		tUnixError = 0;
		freemsg(mp);
		goto ErrorAck;
		/*NOTREACHED*/
		break;

	case T_CHECK:
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: uws: TOptmgmt: doing T_CHECK"));

		if (tOptReq->OPT_length < sizeof(uint32)) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: TOptmgmt opts bad size %d",tOptReq->OPT_length));
			tError= TBADOPT;
			tUnixError = 0;
			freemsg(mp);
			goto ErrorAck;
		}
		if (conEntryPtr->spx2Options) {
			if (tOptReq->OPT_length < VERSION1SPX2OPTSIZE) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uws: TOptmgmt opts bad size %d",tOptReq->OPT_length));
				tError= TBADOPT;
				tUnixError = 0;
				freemsg(mp);
				goto ErrorAck;
			}
		}
		checkFlag = SpxTCheckOptmgmt(conEntryPtr, optReq);
		if (conEntryPtr->spx2Options) {
			spx2OptReq = optReq;
			spx2negotiate = spx2OptReq->spxIIOptionNegotiate;
			spx2retryCount = spx2OptReq->spxIIRetryCount;
			spx2retryDelay = spx2OptReq->spxIIMinimumRetryDelay;
			spx2retryDelta = spx2OptReq->spxIIMaximumRetryDelta;
			spx2windowSize = spx2OptReq->spxIILocalWindowSize;
			spx2sessionFlag = spx2OptReq->spxIISessionFlags;
		} else {
			spxOptReq = optReq;
			retryCount = spxOptReq->spxo_retry_count;
			retryDelay = spxOptReq->spxo_min_retry_delay;
			watchDogFlag = 1;
		}
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: uws: TOptmgmt: T_CHECK returning %d",checkFlag));
		goto PositiveAck;
		/*NOTREACHED*/
		break;

	case T_DEFAULT:
	spx_T_DEFAULT:
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: uws: TOptmgmt: doing T_DEFAULT"));
		if (conEntryPtr->spx2Options) {
			spx2negotiate = SPX2_NEGOTIATE_OPTIONS;
			spx2retryCount = spxMaxTRetries;
			spx2retryDelay = TCKS2MSEC(spxMinTRetryTicks);
			spx2retryDelta = TCKS2MSEC(spxMaxTRetryTicks);
			spx2windowSize = spxWindowSize;
			spx2sessionFlag = spxIpxChecksum;
		} else {
			retryCount = spxMaxTRetries;
			retryDelay = spxMinTRetryTicks;
			watchDogFlag = 1;
		}
		goto PositiveAck;
		/*NOTREACHED*/
		break;

	default:
		tError = TBADFLAG;
		freemsg(mp);
	}

	ErrorAck:
		SpxGenTErrorAck(q, (long) T_OPTMGMT_REQ, tError, tUnixError);
		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: uws: TOptMgmt Error EXIT"));
		NTR_VLEAVE();
		return;

PositiveAck:
	 if (conEntryPtr->spx2Options)
			ackSize = sizeof(SPX2_OPTIONS);
		else
			ackSize = sizeof(SPX_OPTMGMT);

		if ((ackMp = allocb(ackSize + sizeof (struct T_optmgmt_ack),
				 BPRI_MED)) == NULL) {
			spxStats.spx_alloc_failures++; 
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uws: TOptmgmt cant alloc "));
			freemsg(mp);
			NTR_VLEAVE();
			return;
		}
		tOptAck = (struct T_optmgmt_ack *)ackMp->b_rptr;
		tOptAck->PRIM_type = T_OPTMGMT_ACK;
		tOptAck->OPT_length = ackSize;
		tOptAck->OPT_offset = sizeof(struct T_optmgmt_ack);
		tOptAck->MGMT_flags = checkFlag;
		if (conEntryPtr->spx2Options) {
			spx2OptRet = (SPX2_OPTIONS *)(ackMp->b_rptr + tOptAck->OPT_offset);
			spx2OptRet->versionNumber = OPTIONS_VERSION;
			spx2OptRet->spxIIOptionNegotiate = spx2negotiate;
			spx2OptRet->spxIIRetryCount = spx2retryCount;
			spx2OptRet->spxIIMinimumRetryDelay = spx2retryDelay;
			spx2OptRet->spxIIMaximumRetryDelta = spx2retryDelta;
			spx2OptRet->spxIILocalWindowSize = spx2windowSize;
			spx2OptRet->spxIISessionFlags = spx2sessionFlag;
			/* Unsupported Options */
			spx2OptRet->spxIIWatchdogTimeout = TCKS2MSEC(spxWatchEmuInterval);
			spx2OptRet->spxIIConnectionTimeout = 0;
		} else {
			spxOptRet = (SPX_OPTMGMT *)(ackMp->b_rptr + tOptAck->OPT_offset);
			spxOptRet->spxo_retry_count = retryCount;
			spxOptRet->spxo_watchdog_flag = watchDogFlag;
			spxOptRet->spxo_min_retry_delay = retryDelay;
		}

		ackMp->b_wptr = ackMp->b_rptr + sizeof(struct T_optmgmt_ack) 
			+ ackSize; 

		MTYPE(ackMp) = M_PCPROTO;
		qreply(q,ackMp);

		freemsg(mp);
		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: uws: TOptMgmt EXIT"));
		NTR_VLEAVE();
		return;
}

/*
 * int nspxuwput(queue_t *q, mblk_t *mp)
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int 
nspxuwput(queue_t *q, mblk_t *mp)
{
	spxConnectionEntry_t *conEntryPtr;
	struct iocblk *iocp;
	struct linkblk *linkp;
	ipxMctl_t *spxMctlPtr;	
	long tpiType;
	uint8 flushFlags;
	uint32 dsize;
	mblk_t *mp2;
	int	lastMsg;
	int	tli_flag = 0;	/* Set if NON XTI BIND req, affects error codes */


	NTR_ENTER( 2, q, mp, 0, 0, 0);
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE, 
		   "NSPX: uwp: ENTER mp %Xh",mp));

	if ((q == NULL) || (mp == NULL)) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: uwp: q or mp is NULL q %Xh mp %Xh",q, mp));
		if (mp != NULL) freemsg(mp);
		return( NTR_LEAVE(0));
	}

	if (q->q_ptr == NULL) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: uwp: q->q_ptr is NULL "));
		return( NTR_LEAVE(0));
	}

	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;

	switch (MTYPE(mp)) {
	case M_FLUSH:
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					 "NSPX: uwp: SWITCH mp type: M_FLUSH"));

		if (DATA_SIZE(mp) < 1) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uwp: bad MFLUSH size "));
			freemsg(mp);
			break;
		}

		flushFlags = *(mp->b_rptr);

		if (flushFlags & FLUSHW) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uwp: M_IOCTL M_FLUSH Flush Write Side Data"));
			flushq(q, FLUSHDATA);
		}
		if (flushFlags & FLUSHR) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uwp: M_IOCTL M_FLUSH Flush Read Side Data"));
			flushq(RD(q), FLUSHDATA);
			flushFlags &= ~FLUSHW;
			*(mp->b_rptr) = flushFlags;
			qreply(q, mp);
		} else {
			freemsg(mp);
		}
		break;
	
	case M_IOCTL : 
		if (DATA_SIZE(mp) < sizeof(struct iocblk)) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uwp: M_IOCTL bad size %d ",DATA_SIZE(mp)));
			freemsg(mp);
			break;
		}
		spxStats.spx_ioctls++; 
		iocp = (struct iocblk *)mp->b_rptr;
		switch (iocp->ioc_cmd) {
		case I_LINK : 
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					 "NSPX: uwp: M_IOCTL I_LINK "));

			if (conEntryPtr != &spxConnectionTable[0]) { 
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: M_IOCTL I_LINK only daemon can set"));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}

			if (spxbot != NULL) { 
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: M_IOCTL I_LINK spx already linked"));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}
			
			if (mp->b_cont == NULL) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: M_IOCTL I_LINK b_cont is null"));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}

			if (DATA_SIZE(mp->b_cont) < sizeof(struct linkblk)) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: M_IOCTL I_LINK b_cont size bad %d",
					DATA_SIZE(mp->b_cont)));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}

			linkp = (struct linkblk *) mp->b_cont->b_rptr;
			spxbot = linkp->l_qbot;
			spxbot->q_ptr = (caddr_t)&spxConnectionTable[0];
			OTHERQ(spxbot)->q_ptr = (caddr_t)&spxConnectionTable[0];
			MTYPE(mp) = M_IOCACK;
			iocp->ioc_error = 0;
			iocp->ioc_count = 0;
			qreply(q,mp);
			/*
			 * send M_CTL to ipx to get the Internal Net and Node 
			 */
			if ((mp2 = allocb(sizeof(ipxMctl_t), BPRI_HI)) == NULL) {
				spxStats.spx_alloc_failures++; 
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: I_LINK: cant alloc %d",sizeof(ipxMctl_t)));
				iocp->ioc_error = ENOMEM;
				goto iocnak;
			}
			MTYPE(mp2) = M_CTL;
			mp2->b_wptr = mp2->b_rptr + sizeof(ipxMctl_t); 
			spxMctlPtr = (ipxMctl_t *)mp2->b_rptr;	
			spxMctlPtr->mctlblk.cmd = GET_IPX_INT_NET_NODE; 
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					 "NSPX: uwp: M_IOCTL send GET_IPX_INT_NET_NODE to ipx"));

			if (spxbot)
		 		putnext(spxbot,mp2);
			else {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: M_CTL IPX_INT_NET_NODE spxbot is null"));
				freemsg(mp2);
			}
			break;

		case I_UNLINK: 
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					 "NSPX: uwp: M_IOCTL I_UNLINK"));
			spxbot = NULL;
			MTYPE(mp) = M_IOCACK;
			iocp->ioc_error = 0;
			iocp->ioc_count = 0;
			qreply(q, mp);
			break;

		case SPX_GET_CONN_INFO : {
			uint16 connectionNumber;

			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
				"NSPX: uwp: M_IOCTL SPX_GET_CONN_INFO "));

			if (mp->b_cont == NULL) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: GET_CONN_INFO b_cont is NULL"));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}

			GETALIGN16(mp->b_cont->b_rptr, &connectionNumber);
			connectionNumber &= CONIDTOINDEX;

			if (DATA_BSIZE(mp->b_cont) < sizeof(spxConnectionEntry_t)) {
				freemsg(mp->b_cont);
				if ((mp->b_cont = allocb(sizeof(spxConnectionEntry_t),
					BPRI_MED)) == NULL) {
					spxStats.spx_alloc_failures++; 
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					 	"NSPX: uwp: GET_CONN_INFO cant alloc "));
					iocp->ioc_error = EINVAL;
					goto iocnak;
				}
				mp->b_cont->b_wptr =
					 mp->b_cont->b_rptr+sizeof(spxConnectionEntry_t);
				MTYPE(mp->b_cont) = M_DATA;
			}
			
			if (connectionNumber >= spxMaxConnections) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: GET_CONN_INFO connection %d outrange",
					connectionNumber));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}

			if (connectionNumber != 0) {
				bcopy((char *)&spxConnectionTable[connectionNumber],
					(char *)mp->b_cont->b_rptr, sizeof(spxConnectionEntry_t));
			} else {
				bcopy((char *)conEntryPtr, (char *)mp->b_cont->b_rptr,
					sizeof(spxConnectionEntry_t));
			}

			goto iocack;
		}

		case SPX_GS_MAX_PACKET_SIZE : 
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
				"NSPX: uwp: M_IOCTL SPX_GET_MAX_PACKET_SIZE"));
			SpxMaxPacket(q,mp);
			break;
		 
		case SPX_GS_DATASTREAM_TYPE :
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
				"NSPX: uwp: M_IOCTL SPX_SAVE_SEND_HEADER"));
			SpxSaveSendHeader(q,mp);
			break;

		case SPX_T_SYNCDATA_IOCTL :
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
				"NSPX: uwp: M_IOCTL SPX_DATA_IN_TRANSIT"));
			MTYPE(mp) = M_PROTO;
			tpiType = SPX_T_SYNCDATA_REQ;
			bcopy((char *)&tpiType, (char *)mp->b_rptr, sizeof(tpiType));
			putq(q,mp);
			break; 

		case SPX_SPX2_OPTIONS :
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
				"NSPX: uwp: M_IOCTL SPX_SPX2_OPTIONS"));
			conEntryPtr->spx2Options = 1;
			goto iocack;
			/*NOTREACHED*/
			break; 

		case SPX_CHECK_QUEUE :
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
				"NSPX: uwp: M_IOCTL SPX_CHECK_QUEUE"));

			if (mp->b_cont == NULL) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: CHECK_QUEUE b_cont is NULL"));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}

			if (DATA_SIZE(mp->b_cont) < sizeof(uint32)){
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: b_cont msg size to small "));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}

			lastMsg = ((uint16)conEntryPtr->window.local.sequence-1
					 % MAX_SPX2_WINDOW_SIZE);
			if (conEntryPtr->window.unAckedMsgs[lastMsg] != NULL) {
				dsize = msgdsize(conEntryPtr->window.unAckedMsgs[lastMsg]);
				GETALIGN32(&dsize, mp->b_cont->b_rptr);
				goto iocack;
			}

			if ((mp2 = getq(q)) != NULL) {
				dsize = 1;
				GETALIGN32(&dsize, mp->b_cont->b_rptr);
				putbq(q,mp2);
				goto iocack;
			}

			dsize = 0;
			GETALIGN32(&dsize, mp->b_cont->b_rptr);
			goto iocack;
			/*NOTREACHED*/
			break;

		case SPX_GET_STATS :
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
				"NSPX: uwp: M_IOCTL SPX_GET_STATS "));

			if (mp->b_cont == NULL) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: GET_STATS b_cont is NULL"));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}
 
		   	if (DATA_SIZE(mp->b_cont) < sizeof(spxStats_t)){
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: GET_STATS b_cont msg size to small "));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}
			bcopy((char *)&spxStats, (char *)mp->b_cont->b_rptr,
				sizeof(spxStats_t));
 
			goto iocack;
			/*NOTREACHED*/
			break; 

		case SPX_GET_CON_STATS : {
			uint16 connectionNumber;
			spxConStats_t *consp;

			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
				"NSPX: uwp: M_IOCTL SPX_GET_CON_STATS "));

			if (mp->b_cont == NULL) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: GET_CON_STATS b_cont is NULL"));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}
 
		   	if (DATA_SIZE(mp->b_cont) < sizeof(spxConStats_t)){
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: GET_CON_STATS b_cont msg size to small "));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}

			consp = (spxConStats_t *) mp->b_cont->b_rptr;
			connectionNumber = GETINT16(consp->con_connection_id);
			connectionNumber &= CONIDTOINDEX;

			if (connectionNumber >= spxMaxConnections) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: SPX_GET_CON_STATS connection %d outrange",
					connectionNumber));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}

			conEntryPtr = &spxConnectionTable[connectionNumber];

			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: SPX_GET_CON_STATS for connection %x conEntryPtr= %x",
				connectionNumber, conEntryPtr));

			bcopy((char *)&conEntryPtr->conStats, (char *)consp, 
				sizeof(spxConStats_t));

			/* Fill in stats that are not counters, do it every time they
			** might change.
			*/

			IPXCOPYADDR(conEntryPtr->spxHdr.ipxHdr.src.net,
				&consp->con_addr);
			consp->con_connection_id = conEntryPtr->spxHdr.sourceConnectionId;
			consp->con_state = conEntryPtr->state;
			consp->con_retry_count = conEntryPtr->tMaxRetries;
			consp->con_retry_time = TCKS2MSEC(conEntryPtr->minTicksToWait);

			/* if we are in DATAXFER set type, window size, ..etc.   */
			if (consp->con_state >= TS_DATA_XFER) {
				IPXCOPYADDR(conEntryPtr->spxHdr.ipxHdr.dest.net,
					&consp->o_addr);
				consp->o_connection_id = 
					GETINT16(conEntryPtr->spxHdr.destinationConnectionId);
				if (conEntryPtr->protocol & SPX_SPXII)
					consp->con_type |= 2;		/* SPXII session */
				else
					consp->con_type |= 1;		/* SPX session */

				consp->con_window_size = (conEntryPtr->window.local.alloc - 
					conEntryPtr->window.local.ack + 1);
				consp->con_remote_window_size = 
					(conEntryPtr->window.remote.alloc - 
					conEntryPtr->window.remote.ack + 1);

				if (conEntryPtr->ipxChecksum)
					consp->con_ipxChecksum = SPX_SF_IPX_CHECKSUM;
				consp->con_send_packet_size = conEntryPtr->maxTPacketSize;
				consp->con_rcv_packet_size = conEntryPtr->maxRPacketSize;
				consp->con_round_trip_time = 
					TCKS2MSEC(conEntryPtr->lastTickDiff);
			}

			goto iocack;
			/*NOTREACHED*/
			break; 
		}

		case SPX_SET_PARAM : {
			spxParam_t *spxparam;
			spxConnectionEntry_t	*tmpConnTable;
			uint maxConn, maxSockets;
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
				"NSPX: uwp: M_IOCTL SPX_SET_PARAM "));

			if (conEntryPtr != &spxConnectionTable[0]) { 
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: M_IOCTL SPX_SET_PARAM only daemon can set"));
				iocp->ioc_error = EPERM;
				goto iocnak;
			}

			if (mp->b_cont == NULL) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: SET_PARAM b_cont is NULL"));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}

			if (DATA_SIZE(mp->b_cont) < sizeof(spxParam_t)) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uwp: SET_PARAM b_cont msg size to small "));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}
			spxparam = (spxParam_t *) mp->b_cont->b_rptr;
			maxConn = spxparam->spx_max_connections;
			maxSockets = spxparam->spx_max_sockets;
			if ((maxConn < SPX_MIN_CONN) || (maxConn > SPX_MAX_CONN) ||
				(spxMaxConnections != 1)  || /* 0 = no open, > 1 already done*/
				(maxSockets < SPX_MIN_SOCKETS) || 
				(maxSockets > SPX_MAX_SOCKETS )) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"SET_PARAM Invalid Param request %d Conn and %d Sockets",
					maxConn, maxSockets));
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"SET_PARAM Invalid Param spxMaxConnections = %d ",
					spxMaxConnections));
				iocp->ioc_error = EINVAL;
				goto iocnak;
			}
			if (!(tmpConnTable = (spxConnectionEntry_t *)
				kmem_alloc(( maxConn * sizeof(spxConnectionEntry_t)),
					KM_NOSLEEP))) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
					"NSPX: uwp: SET_PARAM: kmem_alloc failed, size %d",
					(maxConn * sizeof(spxConnectionEntry_t))));
				iocp->ioc_error = ENOMEM;
				goto iocnak;
			}
			if (!(spxSocketTable = (spxSocketEntry_t *)
				kmem_alloc(( maxSockets * sizeof(spxSocketEntry_t)),
					KM_NOSLEEP))) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
					"NSPX: uwp: SET_PARAM: kmem_alloc failed, size %d",
					(maxSockets * sizeof(spxSocketEntry_t))));
				kmem_free(tmpConnTable,(maxConn * sizeof(spxConnectionEntry_t)));
				iocp->ioc_error = ENOMEM;
				goto iocnak;
			}
			NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX: uwp: SET_PARAM Max connections=%d, Max Sockets=%d"
				,maxConn,maxSockets));
			spxSocketCount = maxSockets;
			bzero((char*)spxSocketTable, maxSockets * sizeof(spxSocketEntry_t));
			bzero((char*)tmpConnTable, maxConn * sizeof(spxConnectionEntry_t));
			bcopy((char *)spxConnectionTable, (char *)tmpConnTable,
				sizeof(spxConnectionEntry_t));
			kmem_free(spxConnectionTable, sizeof(spxConnectionEntry_t));
			spxConnectionTable = tmpConnTable;
			spxMaxConnections = maxConn;
			spxStats.spx_max_connections = maxConn; 
			q->q_ptr = (caddr_t) &spxConnectionTable[0];
			OTHERQ(q)->q_ptr = (caddr_t) &spxConnectionTable[0];
			goto iocack;
			/*NOTREACHED*/
			break; 
		}

		default:
		
			NWSLOG((SPXID, 0, PNW_SWITCH_DEFAULT, SL_TRACE,
				"NSPX: uwp: M_IOCTL DEFAULT %d",iocp->ioc_cmd));
			iocp->ioc_error = EINVAL;

		iocnak:
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uwp: M_IOCTL IOCNAK %d",iocp->ioc_cmd));

			MTYPE(mp) = M_IOCNAK;
			qreply(q,mp);
			break;

		iocack:
			MTYPE(mp) = M_IOCACK;
			iocp->ioc_error = 0;
			qreply(q, mp);
			break;

		} /* switch ioc->cmd type */
		break;

	case M_PROTO :
		if (DATA_SIZE(mp) < sizeof(long)) {
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uwp: M_PROTO bad data size %d",DATA_SIZE(mp)));
			freemsg(mp);
			break;
		}
		bcopy((char *)mp->b_rptr, (char *)&tpiType, sizeof(tpiType));

		switch (tpiType) {
			case O_T_BIND_REQ :
				tli_flag = 1;
				/*FALLTHRU*/
			case T_BIND_REQ :
				/*
				** Handle the T_BIND_REQ in the uwput routine.  When we call
				** IPX (M_CTL) to allocate a socket IPX will need to allocate
				** memory. On some UNIX platforms (AIX) you need to have process
				** context to allocate memory.  
				*/
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: uwp: M_PROTO T_BIND_REQ"));
				SpxTBindReq(q,mp,tli_flag);
				break;

			default : 
				NWSLOG((SPXID, 0, PNW_SWITCH_DEFAULT, SL_TRACE,
				   "NSPX: uwp: M_PROTO enqueueing to spxuwsrv "));
				spxStats.spx_send_mesg_count++; 
				conEntryPtr->conStats.con_send_mesg_count++; 
				putq(q,mp);
				break;
			}
		break;

	case M_PCPROTO : 
		if (DATA_SIZE(mp) < sizeof(tpiType)) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_ERROR,
					"NSPX: uwp: M_PCPROTO bad data size %d",DATA_SIZE(mp)));
				freemsg(mp);
				break;
		}

		bcopy((char *)mp->b_rptr, (char *)&tpiType, sizeof(tpiType));

		switch(tpiType) {
			case T_INFO_REQ : 
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: uwp: M_PCPROTO T_INFO_REQ"));
				/* since the mp T_info_req is not big enough to reply with we
				   need to allocate memory anyway so free it here */
				freemsg(mp);
				SpxTInfoReq(q);
				break;

			default: 
				NWSLOG((SPXID, 0, PNW_SWITCH_DEFAULT, SL_TRACE,
					"NSPX: uwp: M_PCPROTO DEFAULT tpi type %d",tpiType));
				freemsg(mp);
				break;
			} /* TPI switch */
		break;

	case M_DATA :
		NWSLOG((SPXID, 0, PNW_SWITCH_DEFAULT, SL_TRACE,
			"NSPX: uwp: M_DATA enqueueing for srv"));

		spxStats.spx_send_mesg_count++; 
		conEntryPtr->conStats.con_send_mesg_count++; 
		putq(q,mp);
		break;

	default:
		NWSLOG((SPXID, 0, PNW_SWITCH_DEFAULT, SL_TRACE,
			"NSPX: uwp: MTYPE DEFAULT %d",MTYPE(mp)));

		putq(q,mp);
		break;

	} /* switch MTYPE */

	 NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,  
			   "NSPX: uwp: EXIT"));
	return( NTR_LEAVE(0));
}

/*
 * int nspxuwsrv(queue_t *q) 
 *	Comment to come later.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int 
nspxuwsrv(queue_t *q) 
{
	mblk_t *mp, *ErrorMp;
	long tpiType;
	uint8 tError; 
	uint16 spxDataSize;
	spxConnectionEntry_t *conEntryPtr;
	struct T_error_ack *tErrorAck;
	struct T_ok_ack * tOkAck;
	struct iocblk *iocp;
	int moreData;
	int ret;
	int	tli_flag = 0;	/* Set if NON XTI BIND req, affects error codes */

	NTR_ENTER( 1, q, 0, 0, 0, 0);
	conEntryPtr = (spxConnectionEntry_t *)q->q_ptr;

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE, 
		  "NSPX: uws: ENTER q %Xh",q));

	while ((mp = getq(q)) != NULL) {
		if (spxbot == NULL) {
			/*  
			* Allow unbind requests and disconnect requests even when
			* spxbot has disappeared.  We simply say, "ok" we did it
			* in this case.
			* Also reject T_BIND and T_CONN now, don't wait for fuctions
			* to fail.
			*/
			if (MTYPE(mp) == M_PROTO) {
				bcopy((char *)mp->b_rptr, (char *)&tpiType, sizeof(tpiType));
				switch (tpiType) {
					case T_UNBIND_REQ :
						tOkAck = (struct T_ok_ack *) mp->b_rptr;
						tOkAck->PRIM_type = T_OK_ACK;
						tOkAck->CORRECT_prim = T_UNBIND_REQ;
						mp->b_wptr = mp->b_rptr + sizeof(struct T_ok_ack);
						MTYPE(mp) = M_PCPROTO;
						qreply(q,mp);
						return( NTR_LEAVE(0));
					case T_DISCON_REQ :
						tOkAck = (struct T_ok_ack *) mp->b_rptr;
						tOkAck->PRIM_type = T_OK_ACK;
						tOkAck->CORRECT_prim = T_DISCON_REQ;
						mp->b_wptr = mp->b_rptr + sizeof(struct T_ok_ack);
						MTYPE(mp) = M_PCPROTO;
						qreply(q,mp);
						return( NTR_LEAVE(0));
					case T_CONN_REQ :
					case T_BIND_REQ :
					case O_T_BIND_REQ :
						NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
							"NSPX: uws: Link to IPX broken, ErrorAck Prim %d",
							tpiType));
						tErrorAck = (struct T_error_ack *) mp->b_rptr;
						tErrorAck->PRIM_type = T_ERROR_ACK;
						tErrorAck->ERROR_prim = tpiType;
						tErrorAck->TLI_error = ENONET;
						tErrorAck->UNIX_error = 0;
						mp->b_wptr = mp->b_rptr + sizeof(struct T_error_ack);
						MTYPE(mp) = M_PCPROTO;
						qreply(q,mp);
						return( NTR_LEAVE(0));
					default:
						NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
							"NSPX: uws: Link to IPX broken"));
						tError = ENONET;
						goto ErrorLock;
				}
			}
			NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
				"NSPX: uws: Link to IPX broken"));
			tError = ENONET;
			goto ErrorLock;
		}

		switch (MTYPE(mp)) {
		case M_DATA : 
			/* data sent without M_PROTO header */
			NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
				"NSPX: uws: M_DATA size %d",msgdsize(mp)));

			noenable(q);
			conEntryPtr->disabled = TRUE;

			if ((mp=SpxMData(q, mp, conEntryPtr)) != (mblk_t *)NULL) {
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uws: SpxMData failed"));
				conEntryPtr->disabled = FALSE;
				enableok(q);
				qenable(q);
				goto ReQueue;
			}
			conEntryPtr->disabled = FALSE;
			enableok(q);
			qenable(q);

			conEntryPtr->allocRetryCount = 0; 
			/* if here SpxMData has built T_Data_ind and all
				are on q waiting */

			continue;

		case M_PROTO :
			bcopy((char *)mp->b_rptr, (char *)&tpiType, sizeof(tpiType));

			switch (tpiType) {

			case T_EXDATA_REQ :
			case T_DATA_REQ :
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
						"NSPX: uws: M_PROTO T_DATA_REQ"));

				if (conEntryPtr->state == TS_IDLE) {
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
							"NSPX: uws: T_DATA_REQ TS_IDLE state"));
					spxStats.spx_send_bad_mesg++; 
					conEntryPtr->conStats.con_send_bad_mesg++; 
					freemsg(mp);
					break;
				}

				if ((conEntryPtr->state != TS_DATA_XFER ) &&
					(conEntryPtr->state != TS_WREQ_ORDREL)) {
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
						"NSPX: uws: T_DATA_REQ bad state %d",
						conEntryPtr->state));
					spxStats.spx_send_bad_mesg++;
					conEntryPtr->conStats.con_send_bad_mesg++;
					tError = EPROTO;
					goto ErrorLock;
				}

				if (DATA_SIZE(mp) < sizeof(struct T_data_req)) {
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
						"NSPX: uws: T_DATA_REQ bad header size %d",
						DATA_SIZE(mp)));
					spxStats.spx_send_bad_mesg++; 
					conEntryPtr->conStats.con_send_bad_mesg++;
					tError = EPROTO;
					goto ErrorLock;
				}

				if (!testb( SPXII_PACKET_HEADER_SIZE, BPRI_MED) ) {
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
						"NSPX: uws: T_DATA_REQ testb fail %d",
						   SPX_PACKET_HEADER_SIZE));
					putbq(q,mp);
					goto ReQueue;
				}

				conEntryPtr->allocRetryCount = 0; 
				if (conEntryPtr->protocol & SPX_SPXII)
					spxDataSize = 
						conEntryPtr->maxTPacketSize - SPXII_PACKET_HEADER_SIZE;
				else
					spxDataSize = 
						conEntryPtr->maxTPacketSize - SPX_PACKET_HEADER_SIZE;

				if ((uint16)msgdsize(mp) > spxDataSize) {
 				/* don't let q be enable until done */
					noenable(q);
					conEntryPtr->disabled = TRUE;
					SpxTData(q,mp);
					conEntryPtr->disabled = FALSE;
					enableok(q);
					qenable(q);
					break;
				}

				ret = SpxCheckWindow(conEntryPtr);
				if (ret > 1) {
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
						"NSPX: uws: SpxCheckWindow Failed"));
					putbq(q,mp);
					return( NTR_LEAVE(0));
				}

				/* 
				 * if canputnext failed (ret==1), Send this message but
				 * clear moreData. This will force us to ask/wait for an
				 * acknowledgement. When the ack comes in we will qenable
				 * our queue.
				 */
				if (ret == 1) {
			 		moreData = 0;
				} else {
			 		moreData = qsize(q); 
					if (moreData < 3) {
						moreData = 0;
					}
				}

				SpxTDataReq(q,mp,moreData);
				break;

			case T_CONN_REQ :
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: uws: M_PROTO T_CONN_REQ"));
				spxStats.spx_connect_req_count++;
				SpxTConnReq(q,mp);
				break ;

			case T_CONN_RES :
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: uws: M_PROTO T_CONN_RES"));
				SpxTConnRes(q,mp);
				break;

			case T_DISCON_REQ :
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: uws: M_PROTO T_DISCON_REQ"));
				SpxTDisReq(q,mp);
				break;

			case O_T_BIND_REQ :
				tli_flag = 1;
				/*FALLTHRU*/
			case T_BIND_REQ :
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: uws: M_PROTO T_BIND_REQ"));
				SpxTBindReq(q,mp,tli_flag);
				break;

			case T_UNBIND_REQ :
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: uws: M_PROTO T_UNBIND_REQ"));
				freemsg(mp);
				SpxTUnbindReq(q, (uint8) TRUE);
				break;

			case T_OPTMGMT_REQ :
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: uws: M_PROTO T_OPTMGMT_REQ"));
				SpxTOptmgmt(q,mp);
				break;

#ifndef NW_UP
			case T_ADDR_REQ:
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: uws: M_PROTO T_ADDR_REQ"));
				/*
				 * Since the mp T_addr_req is not big enough to
				 * reply with we need to allocate memory anyway
				 * so free it here.
				 */
				freemsg(mp);
				SpxTAddrReq(q);
				break;
#endif /* NW_UP */

			case SPX_T_SYNCDATA_REQ :
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uws: SPX_T_SYNCDATA_REQ size %d",msgdsize(mp)));
				MTYPE(mp) = M_IOCACK;
				iocp = (struct iocblk *)mp->b_rptr;
				iocp->ioc_cmd = SPX_T_SYNCDATA_IOCTL;
				iocp->ioc_error = 0;
				iocp->ioc_count = 0;
				conEntryPtr->waitOnDataMp = mp;
				mp = mp->b_cont;
				conEntryPtr->waitOnDataMp->b_cont = NULL;

				noenable(q);
				conEntryPtr->disabled = TRUE;
				if ((mp=SpxMData(q, mp, conEntryPtr)) != (mblk_t *)NULL) {
			 		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			 			"NSPX: uws: SpxMData failed"));
					conEntryPtr->disabled = FALSE;
					enableok(q);
					qenable(q);
				 	goto ReQueue;
				 }
				conEntryPtr->disabled = FALSE;
				enableok(q);
				qenable(q);
				conEntryPtr->allocRetryCount = 0; 
			/* if here SpxMData has built T_Data_ind and all
				are on q waiting */

				break;

			case T_UNITDATA_REQ:
				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uws: T_UNITDATA_REQ unsupported"));
				goto NoSupport;

			case T_ORDREL_REQ :
				NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE,
					"NSPX: uws: M_PROTO T_ORDREL_REQ"));
				/* Send Orderly Release Request even if window is closed.
				   This was changed in spxII spec. Feb. 1993
				 */
				SpxTOrdRelReq(q,mp);
				break;

			default : 		
	 			NWSLOG((SPXID, 0, PNW_SWITCH_DEFAULT, SL_TRACE, 
				 "NSPX: uws: M_PROTO DEFAULT",tpiType));

				NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
					"NSPX: uws: TPI prim %d unsupported",tpiType));
				spxStats.spx_unknown_mesg_count++; 
				conEntryPtr->conStats.con_unknown_mesg_count++; 

			NoSupport:
				freemsg(mp);
				if ((ErrorMp = 
					allocb(sizeof(struct T_error_ack), BPRI_HI)) == NULL) {
					spxStats.spx_alloc_failures++; 
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
						"NSPX: uws: Cant alloc > 4"));
						return( NTR_LEAVE(0));
				}
				tErrorAck = (struct T_error_ack *)ErrorMp->b_rptr;
				tErrorAck->PRIM_type = T_ERROR_ACK;
				tErrorAck->ERROR_prim = tpiType;
				tErrorAck->TLI_error = TNOTSUPPORT;
				tErrorAck->UNIX_error = 0;
				ErrorMp->b_wptr = ErrorMp->b_rptr + sizeof(struct T_error_ack);
				MTYPE(ErrorMp) = M_PCPROTO;
				qreply(q,ErrorMp);
				break;

			ReQueue:
				if (conEntryPtr->allocRetryCount >= spxMaxAllocRetries) {	
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
						"NSPX: uws: Requeue reached max alloc retries %d",
						spxMaxAllocRetries));
					SpxGenTDis(conEntryPtr, 
							(long)TLI_SPX_CONNECTION_FAILED, TRUE);
				} else {
					NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
						"NSPX: uws: Requeue requeueing messages %X", mp));
					conEntryPtr->allocRetryCount++;
				}
				return( NTR_LEAVE(0));
					
			} /* M_PROTO case */

			break;

		case M_HANGUP :
		case M_ERROR :
	 		NWSLOG((SPXID, 0, PNW_SWITCH_CASE, SL_TRACE, 
			   "NSPX: uws: M_ERR M_HNG"));
			spxbot = NULL;
			freemsg(mp);
			break;

		default : 
			spxStats.spx_unknown_mesg_count++; 
			conEntryPtr->conStats.con_unknown_mesg_count++; 
	 		NWSLOG((SPXID, 0, PNW_SWITCH_DEFAULT, SL_TRACE, 
				"NSPX: uws: DEFAULT MTYPE %d",MTYPE(mp)));
			freemsg(mp);
			break;
		} /* MTYPE switch */

	} /* while */

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, "NSPX: uws: EXIT")); 

	return( NTR_LEAVE(0));

	ErrorLock:
		MTYPE(mp) = M_ERROR;
		mp->b_rptr = mp->b_wptr = mp->b_datap->db_base;
		bcopy((char *)&tError, (char *)mp->b_wptr, sizeof(tError));
		mp->b_wptr++;
		qreply(q,mp);
				
		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, "NSPX: uws: EXIT Error")); 
		return( NTR_LEAVE(0));
} /* spxuwsrv */

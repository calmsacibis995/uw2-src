/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/spx2.c	1.18"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: spx2.c,v 1.19.2.1.2.1 1995/01/27 15:33:45 meb Exp $"

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
 * GLobal Variables
 */
queue_t				 	*spxbot = NULL;
int 					spxMinorDev = 0;
uint			 		spxMaxConnections = 0;
spxConnectionEntry_t	*spxConnectionTable = NULL;
uint			 		spxSocketCount = 0;
spxSocketEntry_t		*spxSocketTable = NULL;

static	int			spxMajorDev = 0;
static	uint32 		spxInitFlag = 0;
static	uint		spxWatchEmuId = 0;

static const int PACKETSIZES[] =
{
17314,			/* 17k 16 mbit TokenRing */
8192,			/* 16 mbit TokenRing (bandwidth diminishing return point) */
4002,			/* 4k 4mbit TokenRing */
1954,			/* 2k 4mbit TokenRing */
1500,			/* Ethernet 802.3 or Ethernet II */
1492,			/* Ethernet 802.2 */
1474,			/* Ethernet 802.2 SNAP */
1024,			/* Turbo Arcnet */
576,			/* SPX packet size and SPXII minimum packet Size */
0				/* End of struct */
};

/*
 * Forward Declarations:
 */
void SpxPTimeOut(int index);
void dump_mp (mblk_t *m);

/*
 * STREAMS Declarations:
 */
void				nspxinit();
int 				nspxopen(); 
int 				nspxclose(); 
extern	int			nspxlrput(queue_t *q, mblk_t *mp);
extern	int			nspxursrv(queue_t *q);
extern	int			nspxuwput(queue_t *q, mblk_t *mp);
extern	int			nspxuwsrv(queue_t *q);


static struct module_info nspxminfo = { 
	M_SPXID, "nspx", 0, INFPSZ, SPX_HI_WATER_MARK, SPX_LOW_WATER_MARK 
	};

static struct qinit urinit = {
	NULL, nspxursrv, nspxopen, nspxclose, NULL, &nspxminfo, NULL
	};

static struct qinit uwinit = {
	nspxuwput, nspxuwsrv, NULL, NULL, NULL, &nspxminfo, NULL
	};

static struct qinit lrinit = {
	nspxlrput, NULL, NULL, NULL, NULL, &nspxminfo, NULL
	};

static struct qinit lwinit = {
	NULL, NULL, NULL, NULL, NULL, &nspxminfo, NULL
	};

struct streamtab nspxinfo = {
	&urinit, &uwinit, &lrinit, &lwinit
	};

/*
 * void SpxPing(uint16 conId)
 *	Send a watchdog packet to the other endpoint.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxPing(uint16 conId)
{
	register spxConnectionEntry_t *conEntryPtr;

	NTR_ENTER( 1, conId, 0, 0, 0, 0);
	if (conId >= spxMaxConnections) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: Ping: bad connection Id %d",conId));
		NTR_VLEAVE();
		return;
	}

	conEntryPtr = &spxConnectionTable[conId];

	if (conEntryPtr->upperReadQueue == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: Ping: connection %d closed",conId));
		NTR_VLEAVE();
		return;
	}
	conEntryPtr->spxHdr.connectionControl = 
		(SPX_SYSTEM_PACKET | SPX_ACK_REQUESTED);
	conEntryPtr->spxHdr.connectionControl |= conEntryPtr->protocol; 

	if (SpxTranData(conEntryPtr, NULL, (uint8)FALSE,(uint8)FALSE)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: Ping: TranData failed "));
		NTR_VLEAVE();
		return;
	}
	conEntryPtr->conStats.con_send_watchdog++; 
	conEntryPtr->spxHdr.connectionControl = conEntryPtr->protocol; 

	NTR_VLEAVE();
	return;
}

/*
 * void SpxResetConnection(spxConnectionEntry_t *conEntryPtr)
 *	This connection is gone. Cancel all itimout()s for this connection, 
 *	and clear data in the connection structure.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxResetConnection(spxConnectionEntry_t *conEntryPtr)
{
	uint16 sourceSocket, sourceId, i;
	mblk_t *mp;

	NTR_ENTER( 1, conEntryPtr, 0, 0, 0, 0);

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: ENTER Reset Connection"));

	if (conEntryPtr == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: Reset Connection called with NULL ptr"));
		NTR_VLEAVE();
		return;
	}

	if ((conEntryPtr > &spxConnectionTable[spxMaxConnections-1]) ||
		(conEntryPtr < &spxConnectionTable[0])) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: Reset con ptr %Xh out of range ",
			conEntryPtr));
		NTR_VLEAVE();
		return;
	}
		
	if (conEntryPtr->aTimeOutId) {			/* Ack TimeOut */
		untimeout(conEntryPtr->aTimeOutId);
		conEntryPtr->aTimeOutId = 0;
	}

	if (conEntryPtr->pTimeOutId) {			/* Ping TimeOut */
		untimeout(conEntryPtr->pTimeOutId);
		conEntryPtr->pTimeOutId = 0;
	}

	if (conEntryPtr->tTimeOutId) {			/* Transmit TimeOut */
		untimeout(conEntryPtr->tTimeOutId);
		conEntryPtr->tTimeOutId = 0;
	}

	if (conEntryPtr->nTimeOutId) {			/* Negotiation TimeOut */
		untimeout(conEntryPtr->nTimeOutId);
		conEntryPtr->nTimeOutId = 0;
	}

	if (conEntryPtr->waitOnDataMp != NULL) {
		freemsg(conEntryPtr->waitOnDataMp);
		conEntryPtr->waitOnDataMp = NULL;
	}

	IPXCOPYSOCK(&conEntryPtr->spxHdr.ipxHdr.src.sock[0], &sourceSocket);

	GETALIGN16(&conEntryPtr->spxHdr.sourceConnectionId, &sourceId);
	NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
		"NSPX: ResetConnection for socket 0x%x Source ConnectionId %d",
		GETINT16(sourceSocket), GETINT16(sourceId)));

	bzero((char *)&conEntryPtr->spxHdr, sizeof(spxHdr_t));

	GETALIGN16(&sourceId, &conEntryPtr->spxHdr.sourceConnectionId);

	IPXCOPYNET(&IpxInternalNet, conEntryPtr->spxHdr.ipxHdr.src.net);
	IPXCOPYNODE(IpxInternalNode, conEntryPtr->spxHdr.ipxHdr.src.node);
	IPXCOPYSOCK(&sourceSocket, &conEntryPtr->spxHdr.ipxHdr.src.sock[0]);

	conEntryPtr->spxHdr.ipxHdr.chksum = SPX_CHECKSUM;
	conEntryPtr->spxHdr.ipxHdr.tc = 0;
	conEntryPtr->spxHdr.ipxHdr.pt = SPX_PACKET_TYPE;
	conEntryPtr->maxTPacketSize = 
	conEntryPtr->maxRPacketSize = SPX_DEFAULT_PACKET_SIZE;
	conEntryPtr->protocol = ( SPX_SPXII | SPX_NEG ); 
	conEntryPtr->sessionFlags &= NO_NEGOTIATE;
	conEntryPtr->window.remote.ack = 0;
	conEntryPtr->window.remote.alloc = 0;
	conEntryPtr->window.local.sequence = 0;
	conEntryPtr->window.local.ack = 0;
	conEntryPtr->window.local.alloc = 0;
	conEntryPtr->window.local.lastReNegReq_AckNum = 0;
	conEntryPtr->ordRelReqSent = 0; 
	conEntryPtr->ordSeqNumber = 0;


	for (i = 0; i < MAX_SPX2_WINDOW_SIZE; i++) {
		if ((mp = conEntryPtr->window.unAckedMsgs[i]) != NULL) {
			freemsg(mp); 
			conEntryPtr->window.unAckedMsgs[i] = (mblk_t *)NULL;
		}
	}
	for (i = 0; i < MAX_SPX2_WINDOW_SIZE; i++) {
		if ((mp = conEntryPtr->window.outOfSeqPackets[i]) != NULL) {
			freemsg(mp); 
			conEntryPtr->window.outOfSeqPackets[i] = (mblk_t *)NULL;
		}
	}

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: EXIT Reset Connection"));
	NTR_VLEAVE();
	return;
}

/*
 * mblk_t * SpxSetUpPacket(int conId)
 *	Allocate and setup SPX header for a packet going out on the wire.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
mblk_t *
SpxSetUpPacket(int conId)
{
	register spxHdr_t *spxPacket;
	register spxConnectionEntry_t *conEntryPtr;
	spxIIHdr_t *spxIIPacket;
	mblk_t *nmp;

	NTR_ENTER( 1, conId, 0, 0, 0, 0);
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: ENTER SetUpPacket")); 

	if ((conId < 0) || (conId >= spxMaxConnections)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: SetUpPacket bad spx con table index %d",conId));
		return( (mblk_t *)NTR_LEAVE(NULL));
	}
		
	if ((nmp = allocb(SPXII_PACKET_HEADER_SIZE, BPRI_MED))==NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: SetUpPacket cant alloc %d",SPXII_PACKET_HEADER_SIZE));
		spxStats.spx_alloc_failures++; 
		return( (mblk_t *)NTR_LEAVE(NULL));
	}
	conEntryPtr = &spxConnectionTable[conId];
	spxPacket = (spxHdr_t *)nmp->b_rptr; 
	bcopy((char *)&conEntryPtr->spxHdr, (char *)spxPacket, sizeof(spxHdr_t));

	spxPacket->sourceConnectionId = 
		GETINT16(spxPacket->sourceConnectionId);  
	spxPacket->sequenceNumber = 
		GETINT16(conEntryPtr->window.local.sequence);  
	spxPacket->acknowledgeNumber = 
		GETINT16(conEntryPtr->window.local.ack);  
	spxPacket->allocationNumber = GETINT16(conEntryPtr->window.local.alloc);  
	conEntryPtr->window.local.lastAdvertisedAlloc =
		conEntryPtr->window.local.alloc;  

	if ((spxPacket->connectionControl & SPX_SPXII) &&
	   (spxPacket->destinationConnectionId != SPX_CON_REQUEST_CONID)) {
		spxIIPacket = (spxIIHdr_t *)nmp->b_rptr; 
		nmp->b_wptr = nmp->b_rptr + SPXII_PACKET_HEADER_SIZE; 
		spxIIPacket->negotiateSize = GETINT16(conEntryPtr->maxRPacketSize);
	} else
		nmp->b_wptr = nmp->b_rptr + SPX_PACKET_HEADER_SIZE; 

	MTYPE(nmp) = M_DATA;

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: EXIT SetUpPacket")); 

	return((mblk_t *)NTR_LEAVE((int)nmp));
}

/*
 * void SpxSendDisconnect( spxConnectionEntry_t *conEntryPtr, uint8 retry )
 *	Send a disconnect packet to othe endpoint.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxSendDisconnect( spxConnectionEntry_t *conEntryPtr, uint8 retry )
{

	NTR_ENTER( 1, conEntryPtr, 0, 0, 0, 0);
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: senddis: ENTER conEntryPtr %Xh",conEntryPtr));
		
	if (conEntryPtr == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: senddis: conEntryPtr is NULL"));
		NTR_VLEAVE();
		return;
	}

/* assumes memory is contiguos should be in kernel*/
	if ((conEntryPtr < &spxConnectionTable[0]) || 
		(conEntryPtr > &spxConnectionTable[(spxMaxConnections-1)])) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: senddis: conEntryPtr %Xh is out of range",
			conEntryPtr));
		NTR_VLEAVE();
		return;
	}

	if ((conEntryPtr->state != TS_DATA_XFER) &&
		(conEntryPtr->state != TS_WREQ_ORDREL) &&
		(conEntryPtr->state != TS_WRES_CIND) &&
		(conEntryPtr->state != TS_WIND_ORDREL)) {
		NWSLOG(( SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: senddis: no remote address, state %d ",conEntryPtr->state));
		NTR_VLEAVE();
		return;
	}

	if (spxbot == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: senddis: TranData link to ipx is broken"));
		NTR_VLEAVE();
		return;
	}

	conEntryPtr->spxHdr.dataStreamType = (uint8) SPX_TERMINATE_REQUEST;
	conEntryPtr->spxHdr.connectionControl = conEntryPtr->protocol; 
	conEntryPtr->spxHdr.connectionControl |= SPX_ACK_REQUESTED;	

	if (SpxTranData(conEntryPtr, NULL, retry,(uint8)TRUE)) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: uws: SendDisconnect: TranData failed"));
		NTR_VLEAVE();
		return;
	}
	conEntryPtr->spxHdr.dataStreamType = 0;
	conEntryPtr->spxHdr.connectionControl = conEntryPtr->protocol; 

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: senddis: EXIT ")); 

	NTR_VLEAVE();
	return;
}

/*
 * void SpxGenTDis(spxConnectionEntry_t *conEntryPtr, long reason, int dre)
 *	Notify endpoint and user of disconnect the close connection.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxGenTDis(spxConnectionEntry_t *conEntryPtr, long reason, int dre)
{
	struct T_discon_ind *tDisconInd;
	mblk_t *flushMp, *mp;
	struct iocblk *iocp;

	NTR_ENTER( 3, conEntryPtr, reason, dre, 0, 0);
	if (conEntryPtr == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: GenTDis: conEntryPtr is NULL"));
		NTR_VLEAVE();
		return;
	}

/* assumes memory is contiguos should be in kernel*/
	if ((conEntryPtr < &spxConnectionTable[0]) || 
		(conEntryPtr > &spxConnectionTable[(spxMaxConnections-1)])) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: GenTDis: conEntryPtr %Xh is out of range",
			conEntryPtr));
		NTR_VLEAVE();
		return;
	}

	if (conEntryPtr->upperReadQueue == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: GenTDis: connection is closed"));
		NTR_VLEAVE();
		return;
	}

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
			"NSPX: GenTDis: ENTER "));

	if (dre) {
		SpxSendDisconnect( conEntryPtr, (uint8)FALSE );
	} else {
		if (reason != (uint8) TLI_SPX_CONNECTION_TERMINATED)
			spxStats.spx_abort_connection++; 
	}
	
	if (conEntryPtr->waitOnDataMp) {
		iocp = (struct iocblk *)conEntryPtr->waitOnDataMp->b_rptr;
		iocp->ioc_error = ENONET;
		MTYPE(conEntryPtr->waitOnDataMp) = M_IOCNAK;
		putnext(conEntryPtr->upperReadQueue, conEntryPtr->waitOnDataMp);
		conEntryPtr->waitOnDataMp = NULL;
	}

	conEntryPtr->state = TS_IDLE;

	if ((flushMp = allocb(1, BPRI_MED)) == NULL ) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: GenTDis: cant alloc flush msg size 1"));
	} else {
		*(flushMp->b_rptr) = FLUSHR | FLUSHW; /* flushFlags */
		MTYPE(flushMp) = M_FLUSH;
		flushMp->b_wptr = flushMp->b_rptr + 1;
		putnext(conEntryPtr->upperReadQueue,flushMp);
	}

	if ((mp=allocb(sizeof(struct T_discon_ind), BPRI_MED)) == NULL ) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: GenTDis: cant alloc %d msg",
			sizeof(struct T_discon_ind)));
		NTR_VLEAVE();
		return;
	}
		
	tDisconInd = (struct T_discon_ind *)mp->b_rptr;
	tDisconInd->PRIM_type = T_DISCON_IND;
	tDisconInd->DISCON_reason = reason;
	tDisconInd->SEQ_number = 0; /* need to fill right seq # */
	mp->b_wptr = mp->b_rptr + sizeof(struct T_discon_ind);
	MTYPE(mp) = M_PROTO;

	if (conEntryPtr->responseQueue) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: GenTDis: Send T_DISCON_IND to resQue 0x%x",
			conEntryPtr->responseQueue));
		IPXCOPYSOCK(&conEntryPtr->acceptSocket,
			&conEntryPtr->spxHdr.ipxHdr.src.sock);
		/*
		 * if timeout on server connection establishment,
		 * send T_DISCON_IND up on the same queue that 
		 * T_CONN_RES came down on
		 */
		qreply(conEntryPtr->responseQueue,mp);
		conEntryPtr->responseQueue = NULL;
	} else {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: GenTDis: Send T_DISCON_IND to q 0x%x ",
			conEntryPtr->upperReadQueue));
		putnext(conEntryPtr->upperReadQueue,mp);
	}

	qenable(WR(conEntryPtr->upperReadQueue));

	SpxResetConnection(conEntryPtr);

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: GenTDis: EXIT "));
	NTR_VLEAVE();
	return;
}

/*
 * void SpxWatchEmu()
 *	Send a watchdog packet to all connections that we have not heard
 *	from lately.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxWatchEmu()
{
	register spxConnectionEntry_t *conEntryPtr;
	uint i;
	register uint32 tickDiff;
	uint32 currentTicks;

	NTR_ENTER( 0, 0, 0, 0, 0, 0);
	drv_getparm(LBOLT, (void *)&currentTicks);

	spxWatchEmuId = 0;
	for (i=1; i<spxMaxConnections; i++) {
		conEntryPtr = &spxConnectionTable[i];

		/* Check the connection if not renegotiating and in DATAXFER,
		   WAITING-ORD-REL-REQ, WAITING-ORD-REL-IND state or
		   initial negotiation. */
		if ((conEntryPtr->upperReadQueue != NULL) &&
			(!(conEntryPtr->sessionFlags & RE_NEGOTIATE)) &&
			((conEntryPtr->state == TS_DATA_XFER) ||
			(conEntryPtr->state == TS_WREQ_ORDREL) ||
			(conEntryPtr->state == TS_WIND_ORDREL) ||
			(conEntryPtr->sessionFlags & 
				(uint16)~(NO_NEGOTIATE | RE_NEGOTIATE)))) {

			if (spxbot == NULL) {
				NWSLOG((SPXID, i, PNW_ERROR, SL_ERROR,
					"NSPX: WatchEmu: spxbot is NULL Killing connection"));
				SpxGenTDis(conEntryPtr, 
					(long) TLI_SPX_CONNECTION_FAILED, FALSE);
				continue;
			}

			NWSLOG((SPXID, i, PNW_ERROR, SL_TRACE,
				"NSPX: WatchEmu: curticks %d conticks %d",
				currentTicks, conEntryPtr->remoteActiveTicks));

			if (currentTicks >= conEntryPtr->remoteActiveTicks)
				tickDiff = 
					currentTicks - conEntryPtr->remoteActiveTicks;
			else
			/* won't rollover till after a year of no boots */
				tickDiff = (uint32)0xFFFFFFFF -
					(conEntryPtr->remoteActiveTicks - currentTicks);
			
			if ((tickDiff >= spxWatchEmuInterval) && 
					(!(conEntryPtr->pTimeOutId))) {  
				NWSLOG((SPXID, i, PNW_ERROR, SL_TRACE,
					"NSPX: WatchEmu: Pinging Conn next %d ",
					spxWatchEmuInterval));
				conEntryPtr->pRetries = 0;
				conEntryPtr->pTimeOutId = itimeout(SpxPTimeOut,
					(void *)i, conEntryPtr->tTicksToWait, pltimeout);
				SpxPing(i);
			}
		}	/* if there is a connection */
	} /* for loop */

	spxWatchEmuId = itimeout(SpxWatchEmu, (void *)0, spxWatchEmuInterval, pltimeout);
	NTR_VLEAVE();
	return;
}

/*
 * void SpxSetOptions(spxConnectionEntry_t *conEntryPtr, mblk_t *mp)
 *	Set option in Seesion Setup or Negotiate packet.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxSetOptions(spxConnectionEntry_t *conEntryPtr, mblk_t *mp)
{
	register negoHdr_t *negotPacket;
	uint32 usecToNet;

	NTR_ENTER( 2, conEntryPtr, mp, 0, 0, 0);
	if (conEntryPtr == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: SpxSetOptions called with NULL ptr")); 
		NTR_VLEAVE();
		return;
	}
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: ENTER SetOptions")); 

	/* Zero out first part of message*/
	bzero(mp->b_rptr,48);
	negotPacket = (negoHdr_t *)mp->b_rptr; 
	negotPacket->numberOfOptions = GETINT16(NUMBER_OF_OPTIONS);	

/* Set Round Trip Time option */
	negotPacket->RttOpt.TypeSize = SPXII_RTT;
	if(conEntryPtr->ticksToNet < 1)
		conEntryPtr->ticksToNet = 1;  
	usecToNet = ((conEntryPtr->ticksToNet * 1000000)/HZ );
	usecToNet = GETINT32(usecToNet);
	bcopy((char *)&usecToNet,(char *)&negotPacket->RttOpt.Value,sizeof(uint32));

/* Set IPX Checksum option */
	negotPacket->ChksmOpt.TypeSize = IPX_CHKSM;
	if (conEntryPtr->ipxChecksum) {
		negotPacket->ChksmOpt.Value = (uint8)0xbd;
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: SetOptions: Turn IPX Checksums ON"));
	} else {
		negotPacket->ChksmOpt.Value = (uint8)0x0;
		NWSLOG((SPXID, 0, PNW_ASSIGNMENT, SL_TRACE,
			"NSPX: SetOptions: Turn IPX Checksums OFF"));
	}

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: SpxSetOptions EXIT"));
	NTR_VLEAVE();
	return;
}

/*
 * void SpxSendNegSessPkt(spxConnectionEntry_t *)
 * Send a Negotiate Size packet or a Session Negotiate packet.
 * The type of the packet is already determined by sessionFlags set
 * by the calling routine.  The size of the packet is determined by
 * maxTPacketSize in the connection Table.
 *
 * Calling/Exit State:
 *	No locks set on entry or exit.
 */
void
SpxSendNegSessPkt(spxConnectionEntry_t *conEntryPtr)
{
	mblk_t *mp = NULL;
	mblk_t *spxmp;
	spxIIHdr_t *spxPacket; 
	uint conId,maxDataSize;
	uint32 tickDiff;
	int		i, packetSize;

	NTR_ENTER( 1, conEntryPtr, 0, 0, 0, 0);
	if (conEntryPtr == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: SpxSendNegSessPkt called with NULL ptr")); 
		NTR_VLEAVE();
		return;
	}
	conId = conEntryPtr->spxHdr.sourceConnectionId;
	conId &= CONIDTOINDEX;
	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: ENTER NegotiateSize")); 
	conEntryPtr->tTimeOutId = 0;

	if (!canputnext(spxbot)) {
		conEntryPtr->conStats.con_canput_fail++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: NegotiateSize cant put down to q %Xh",spxbot));
		goto Reschedule;
	}
	if (conEntryPtr->sizeNegoRetry >= conEntryPtr->tMaxRetries ) { 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE,
			"NSPX: NegotiateSize: reached max retries %d closing connection",
				conEntryPtr->tMaxRetries));
		spxStats.spx_max_retries_abort++; 
		SpxGenTDis(conEntryPtr, (long) TLI_SPX_CONNECTION_FAILED, FALSE);
		goto Exit;
	}

	if (conEntryPtr->sizeNegoRetry) {

	/* find the next smaller packet size */
		for( i=0; i < sizeof(PACKETSIZES)/sizeof(int); i++) {
			packetSize = PACKETSIZES[i];
			if (packetSize < conEntryPtr->maxTPacketSize)
				break; 
		}

		if (packetSize < SPX_DEFAULT_PACKET_SIZE)
			packetSize = SPX_DEFAULT_PACKET_SIZE;

		conEntryPtr->maxTPacketSize = packetSize;
	}
	maxDataSize = (conEntryPtr->maxTPacketSize - SPXII_PACKET_HEADER_SIZE);
	if ((mp = allocb(maxDataSize,BPRI_MED)) == NULL ) {
		spxStats.spx_alloc_failures++; 
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: NegotiateSize: cant alloc %d",maxDataSize));
		goto Reschedule;
	}
	MTYPE(mp) = M_DATA;

	if (conEntryPtr->sessionFlags & SESS_SETUP) {
		/*copy accept socket to address.sock, use accept socket from now on	*/
		IPXCOPYSOCK(&conEntryPtr->acceptSocket,
			&conEntryPtr->spxHdr.ipxHdr.src.sock);
	}

	mp->b_wptr = mp->b_rptr + maxDataSize;
	if(conEntryPtr->sessionFlags & RE_NEGOTIATE) {
		bzero(mp->b_rptr,8);
	} else
		SpxSetOptions(conEntryPtr,mp);
	conEntryPtr->spxHdr.connectionControl = conEntryPtr->protocol; 

	if ((spxmp = SpxSetUpPacket(conId)) == NULL ) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: SpxSendNegSessPkt : SetUpPacket failed"));
		goto Reschedule;
	}
	spxPacket = (spxIIHdr_t *)spxmp->b_rptr; 
	spxPacket->connectionControl |=	
			(SPX_SYSTEM_PACKET | SPX_ACK_REQUESTED | SPX_NEG);
	if ((conEntryPtr->sessionFlags & SESS_SETUP) ||
		(conEntryPtr->sessionFlags & RE_NEGOTIATE))
		spxPacket->negotiateSize = GETINT16(conEntryPtr->maxRPacketSize);	
	else
		spxPacket->negotiateSize = GETINT16(conEntryPtr->maxSDU);	

	spxPacket->ipxHdr.len = GETINT16(msgdsize(mp) +  msgdsize(spxmp));
	spxmp->b_cont = mp;

	/* if re-negotiating give Max time for ACK */
	if(conEntryPtr->sessionFlags & RE_NEGOTIATE) {
		tickDiff = conEntryPtr->maxTicksToWait;
	} else { 
		tickDiff = conEntryPtr->ticksToNet;
		if (tickDiff < 4)
			tickDiff = 4;
		tickDiff += (tickDiff >> 1);
	}

	conEntryPtr->sessionFlags |= WAIT_NEGOTIATE_ACK;
	conEntryPtr->tTimeOutId = itimeout(SpxSendNegSessPkt, 
		(void *)conEntryPtr, tickDiff, pltimeout);
	putnext(spxbot,spxmp);
	conEntryPtr->sizeNegoRetry++;

	Exit:
		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
				"NSPX: NegotiateSize: EXIT"));
		NTR_VLEAVE();
		return;

		/* Cannot put or allocate try again later */
	Reschedule:
		if (mp) 
			freemsg(mp);
		conEntryPtr->sizeNegoRetry++;
		tickDiff = (conEntryPtr->ticksToNet * 2);
		conEntryPtr->tTimeOutId = itimeout(SpxSendNegSessPkt,
				(void *)conEntryPtr, tickDiff, pltimeout);
		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
				"NSPX: NegotiateSize: EXIT Reschedule"));
		NTR_VLEAVE();
		return;
}

/*
 * void nspxinit()
 *	Initialize stuff
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
nspxinit()
{
	NTR_ENTER( 0, 0, 0, 0, 0, 0);
	/*
	 *+ Inform that driver is loaded
	 */
#ifdef DEBUG
	cmn_err(CE_CONT, "%s %s%s: %s %s\n",
						SPXSTR, SPXVER, SPXVERM, __DATE__, __TIME__);
#else
	cmn_err(CE_CONT, "%s %s%s\n", SPXSTR, SPXVER, SPXVERM);
#endif

	bzero((char *)&spxStats, sizeof(spxStats_t));
	spxStats.spx_max_connections = (uint16)spxMaxConnections;
	spxStats.spx_major_version = (uint16)SPX_MAJOR;
	spxStats.spx_minor_version = (uint16)SPX_MINOR;
	spxStats.spx_revision[0] = (char)SPX_REV1;
	spxStats.spx_revision[1] = (char)SPX_REV2;


	spxInitFlag = SPX_INIT_RUN_FLAG;
	spxWatchEmuInterval *= (HZ);
	spxMinTRetryTicks = MSEC2TCKS(spxMinTRetryTicks);
	spxMaxTRetryTicks *= (HZ);
	spxWatchEmuId = 0;
	spxbot = NULL;
	spxConnectionTable = NULL;
	spxSocketTable = NULL;
	NTR_VLEAVE();
	return;
}

/*
 * int SpxDevicesOpen()
 *	Return the number of SPX devices open including the control device.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
int
SpxDevicesOpen()
{
	int i;
	int count;

	NTR_ENTER( 0, 0, 0, 0, 0, 0);
	count = 0;
	for (i=0; i < spxMaxConnections; i++)
		if (spxConnectionTable[i].upperReadQueue != NULL)
			count++;
	
	return( NTR_LEAVE(count));
}

/*
 * int nspxopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *credp)
 *	Open and setup and SPX device.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
/*ARGSUSED*/
int 
nspxopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	register mblk_t			*mop;
	int 					minorNo;
	spxConnectionEntry_t 	*conEntryPtr;
	struct 					stroptions *sop;
	int						spxMajor;
	int						stillOpen;
	uint16					connectionId;
	uint32					time;

	NTR_ENTER( 5, q, *devp, flag, sflag, credp);
	NWSLOG((SPXID,spxMinorDev,PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: open: ENTER rq %Xh wq %Xh",q,WR(q)));

	if (spxInitFlag != SPX_INIT_RUN_FLAG) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE,
			"NSPX: open: spxinit has not been executed"));
		spxStats.spx_open_failures++;
		return( NTR_LEAVE(ENXIO));
	}

	if ((mop = allocb(sizeof(struct stroptions), BPRI_MED))
		== NULL ) {
		NWSLOG((SPXID, spxMinorDev, PNW_ERROR, SL_TRACE, 
			"NSPX: open: cant alloc %d",sizeof(struct stroptions)));
		spxStats.spx_alloc_failures++; 
		spxStats.spx_open_failures++;
		return( NTR_LEAVE(EAGAIN));
	}

	MTYPE(mop) = M_SETOPTS;
	mop->b_wptr += sizeof (struct stroptions);
	sop = (struct stroptions *)mop->b_rptr;
	sop->so_flags = SO_HIWAT | SO_LOWAT;
	sop->so_hiwat = SPX_HI_WATER_MARK;
	sop->so_lowat = SPX_LOW_WATER_MARK;
	putnext(q,mop);
	spxMajor = getemajor(*devp);
	drv_getparm(TIME, &time);

	if (sflag == CLONEOPEN) {
		for (minorNo = 1; minorNo < spxMaxConnections; minorNo++)
			if (spxConnectionTable[minorNo].upperReadQueue == NULL)
				break;
		*devp = makedevice(spxMajor,minorNo);
	} else {
		minorNo = getminor(*devp);

		if (minorNo == 0) {
			/* Only allow root to open control device. */
			if (drv_priv(credp) != 0) {
				NWSLOG(( SPXID,0,PNW_ERROR,SL_TRACE,
					"nspxopen: non root attempt to open control device"));
				spxStats.spx_open_failures++;
				return( NTR_LEAVE( EPERM));
			}
			if ((stillOpen = SpxDevicesOpen()) > 0 ) {
				NWSLOG(( SPXID,0,PNW_ERROR,SL_TRACE,
					"nspxopen:  %d devices still open, reject open of nspx0",
					stillOpen));
				spxStats.spx_open_failures++;
				return( NTR_LEAVE( EAGAIN));
			}
		}

		if (q->q_ptr) {
			NWSLOG((SPXID, minorNo, PNW_ALL, SL_TRACE,  
				"NSPX: open:  Multiple open on minorNo= 0x%x",
				minorNo));
			/*
			 * Set Flag indicating multiple open (NVT)
			 */
			conEntryPtr = (spxConnectionEntry_t *) q->q_ptr;
			conEntryPtr->conStats.con_type = 0x80;
			goto Exit;
		} else if ( minorNo != 0 ) {
			NWSLOG((SPXID, minorNo, PNW_ERROR, SL_TRACE,  
				"NSPX: open: Non-clone first open on minor %x ",
				minorNo));
			spxStats.spx_open_failures++;
			return( NTR_LEAVE( EAGAIN));
		}

		/* Save time of first open */
		spxStats.spx_startTime = time;
		spxStats.spx_current_connections = 0;

	}
	NWSLOG((SPXID, minorNo ,PNW_ASSIGNMENT, SL_TRACE,
		"NSPX: open: Minor device = %d Major device = %d",
		minorNo,spxMajor));

	if ((spxbot == NULL) && (minorNo != 0)) {
		NWSLOG((SPXID, minorNo, PNW_ERROR, SL_TRACE,
			"NSPX: open: SPX not linked to IPX"));
		spxStats.spx_open_failures++;
		return( NTR_LEAVE(ENXIO));
	}

	if (spxMaxConnections && (minorNo >= spxMaxConnections)) { 
		NWSLOG((SPXID, minorNo, PNW_ERROR, SL_TRACE, 
			"NSPX: open: device out of range %d",minorNo));
		spxStats.spx_open_failures++;
		return( NTR_LEAVE(EAGAIN));
	}

	/* Allocate space for dev 0.		*/
	if ((spxMaxConnections == 0) && (minorNo == 0)) {
		if (!(spxConnectionTable = (spxConnectionEntry_t *)
				kmem_alloc(sizeof(spxConnectionEntry_t), KM_NOSLEEP))) {
			NWSLOG((SPXID, minorNo, PNW_ERROR, SL_TRACE, 
				"NSPX: open: cant kmem_alloc %d",sizeof(spxConnectionEntry_t)));
			spxStats.spx_open_failures++;
			return( NTR_LEAVE(EAGAIN));
		}
		spxMaxConnections = 1;
		bzero((char *)spxConnectionTable,sizeof(spxConnectionEntry_t));
		spxMajorDev = spxMajor;		/* save major Number of control device */
	}

	if ((uint16)minorNo >= spxStats.spx_max_used_connections)
		spxStats.spx_max_used_connections = ((uint16)minorNo + 1);

	spxStats.spx_current_connections++;


	/*
	 * The connectionId lower 10 bits is comprised by the minorNo
	 * and hence the index into spxConnectionTable for this entry.
	 * The connectionId upper 6 bits is comprised of an unique number
	 * designed to be chosen randomly at startup and then
	 * incremented each time this conEntryPtr is used to establish
	 * a new connection.
	 *
	 * (time % spxMaxConnections + #of opens) is a semi-random number
	 */
	connectionId = (((time % spxMaxConnections)) << 10) + minorNo;

	conEntryPtr = SpxIDtoEntry(connectionId);
	conEntryPtr->conStats.con_type = 0;
	conEntryPtr->conStats.con_startTime = time;
	conEntryPtr->upperReadQueue = q;
	conEntryPtr->spxHdr.ipxHdr.chksum = SPX_CHECKSUM;
	conEntryPtr->spxHdr.ipxHdr.tc = 0;
	conEntryPtr->spxHdr.ipxHdr.pt = SPX_PACKET_TYPE;
	conEntryPtr->spxHdr.sourceConnectionId = connectionId;
	conEntryPtr->state = TS_UNBND;
	conEntryPtr->tMaxRetries = spxMaxTRetries;
	conEntryPtr->tTicksToWait = spxMinTRetryTicks;
	conEntryPtr->minTicksToWait = spxMinTRetryTicks;
	conEntryPtr->maxTicksToWait = spxMaxTRetryTicks;
	conEntryPtr->ticksToNet = spxMinTRetryTicks;
	drv_getparm(LBOLT, (void *)&conEntryPtr->beginTicks);
	conEntryPtr->endTicks = conEntryPtr->beginTicks;
	conEntryPtr->remoteActiveTicks = conEntryPtr->beginTicks;
	conEntryPtr->maxTPacketSize = 
	conEntryPtr->maxRPacketSize = SPX_DEFAULT_PACKET_SIZE;
	conEntryPtr->protocol = ( SPX_SPXII | SPX_NEG ); 
	conEntryPtr->waitOnDataMp = NULL;
	conEntryPtr->specialFlags = 0;
	conEntryPtr->goingUpCount = 0;
	conEntryPtr->flowControl = FALSE;
	conEntryPtr->ipxChecksum = spxIpxChecksum;
	conEntryPtr->ordRelReqSent = 0; 
	conEntryPtr->ordSeqNumber = 0;
	conEntryPtr->spxWinSize = spxWindowSize;
	conEntryPtr->window.remote.ack = 0;
	conEntryPtr->window.remote.alloc = 0;
	conEntryPtr->window.local.sequence = 0;
	conEntryPtr->window.local.ack = 0;
	conEntryPtr->window.local.alloc = 0;
	conEntryPtr->window.local.lastReNegReq_AckNum = 0;

	/* if major number is tha same as the control device use SPX_OPTS,
	   else use SPX2_OPTIONS */
	if (spxMajor == spxMajorDev)
		conEntryPtr->spx2Options = 0;
	else
		conEntryPtr->spx2Options = 1;

	q->q_ptr = (caddr_t) conEntryPtr; 
	WR(q)->q_ptr = (caddr_t) conEntryPtr;

	/* if first device start watchemu */

	NWSLOG((SPXID, minorNo, PNW_ASSIGNMENT, SL_TRACE,
		"NSPX: open: spxDevicesOpen = %d",SpxDevicesOpen()));

	if (SpxDevicesOpen() > 0 ) {
		if ((!spxWatchEmuId) && (spxWatchEmuInterval)) {
			spxWatchEmuId = itimeout(SpxWatchEmu, (void *)0, 
				spxWatchEmuInterval, pltimeout);
			NWSLOG((SPXID, minorNo, PNW_ASSIGNMENT, SL_TRACE,
				"NSPX: open: enqueued WatchEmu id %d",spxWatchEmuId));
		}
	}

Exit:
	NWSLOG((SPXID, minorNo, PNW_EXIT_ROUTINE, SL_TRACE,  
				"NSPX: open: EXIT good open"));

	return( NTR_LEAVE(0));
}

/*
 * int nspxclose(queue_t *q, int flag, cred_t *credp)
 *	Close and spx devive. iDisconnect and unbind if need
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
/*ARGSUSED*/
int 
nspxclose(queue_t *q, int flag, cred_t *credp)
{
	spxConnectionEntry_t *conEntryPtr;
	spxConnectionEntry_t *conPtr;
	uint16 conId,i,j;
	uint16	socket;
	mblk_t  *mp;
	queue_t *wrQueue;

	NTR_ENTER( 3, q, flag, credp, 0, 0);

	NWSLOG((SPXID, 0, PNW_ENTER_ROUTINE, SL_TRACE, 
		"NSPX: close: ENTER spxclose procedure"));

	if (q->q_ptr == NULL) {
		NWSLOG((SPXID, 0, PNW_ERROR, SL_TRACE, 
			"NSPX: close: q->q_ptr is NULL, error exit"));
		return( NTR_LEAVE(EFAULT));
	}
	conEntryPtr = (spxConnectionEntry_t *) q->q_ptr;
	conId = conEntryPtr->spxHdr.sourceConnectionId;
	NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE, 
		"NSPX: close: Closing conId 0x%X", conId));

/* to avoid a race conditions w TimeOut/GotAck/RDisC set this first */
	conEntryPtr->upperReadQueue = NULL;

	if (conEntryPtr->aTimeOutId) {
		untimeout(conEntryPtr->aTimeOutId);
		conEntryPtr->aTimeOutId = 0;
	}
	if (conEntryPtr->pTimeOutId) {
		untimeout(conEntryPtr->pTimeOutId);
		conEntryPtr->pTimeOutId = 0;
	}
	if (conEntryPtr->tTimeOutId) {
		untimeout(conEntryPtr->tTimeOutId);
		conEntryPtr->tTimeOutId = 0;
	}
	if (conEntryPtr->nTimeOutId) {
		untimeout(conEntryPtr->nTimeOutId);
		conEntryPtr->nTimeOutId = 0;
	}
	if (conEntryPtr->waitOnDataMp != NULL) {
			freemsg(conEntryPtr->waitOnDataMp);
			conEntryPtr->waitOnDataMp = NULL;
	}

	/*
	** Only send Disconnect if we have a destination socket
	*/
	IPXCOPYSOCK( &conEntryPtr->spxHdr.ipxHdr.dest.sock, &socket);
	if(socket) {
		SpxSendDisconnect(conEntryPtr, (uint8)FALSE);
	}

	SpxTUnbindReq(q, (uint8) FALSE);

/* If SpxTUnbindReq fails because of wrong state, spxSocketTable
   will not be cleared.  We are going away so do it now if needed.   */

	for (i=0; i < spxSocketCount; i++)
		if (conId == spxSocketTable[i].listenConnectionId)
		break;

	if (i < spxSocketCount) {
		NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE,
		"NSPX: Close: Listening ConnId %d found, clearing",conId));
		/*
		 * If we are a listener, check and see if our write queue
		 * is being used for connection responses by any other device.
		 */ 
		wrQueue = WR(q);
		for (j=0; j < spxSocketCount; j++) {
			if (wrQueue == spxConnectionTable[j].responseQueue) {
				NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE,
					"NSPX: Close: NULL responseQueue for conID %d",
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
		NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE,
		"NSPX: Close: NOT listening on ConnId %d ",conId));
	}
#endif

	for (i = 0; i < MAX_SPX2_WINDOW_SIZE; i++) {
		if ((mp = conEntryPtr->window.unAckedMsgs[i]) != NULL) {
			freemsg(mp); 
			conEntryPtr->window.unAckedMsgs[i] = (mblk_t *)NULL;
		}
	}
	for (i = 0; i < MAX_SPX2_WINDOW_SIZE; i++) {
		if ((mp = conEntryPtr->window.outOfSeqPackets[i]) != NULL) {
			freemsg(mp); 
			conEntryPtr->window.outOfSeqPackets[i] = (mblk_t *)NULL;
		}
	}
	/*
	** if SpxTUnbindReq fails because of wrong state, release IPX
	** socket here.
	*/
	IPXCOPYSOCK( &conEntryPtr->spxHdr.ipxHdr.src.sock, &socket);
	if(socket) {
		SpxRelIpxSocket(GETINT16(socket));
	}

	bzero((char *)conEntryPtr,sizeof(spxConnectionEntry_t));

	conId &= CONIDTOINDEX;
	NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE,
		"NSPX: close: closing conId 0x%X", conId));
	if (conId == 0) {
		NWSLOG((SPXID, 0, PNW_ALL, SL_TRACE,
			"NSPX: close: closing the control Device"));
		/* if daemon dies shut down spx */
		for (i=1; i<spxMaxConnections; i++) {
			if (spxConnectionTable[i].upperReadQueue == NULL) { 
				continue;
			}
			conPtr = &spxConnectionTable[i];
			NWSLOG((SPXID, i, PNW_EXIT_ROUTINE, SL_TRACE,
				"NSPX: close: Sending M_HANGUP for Connection ID %d Ptr=%x",
				 i,conPtr));
			putctl(conPtr->upperReadQueue->q_next, M_HANGUP); 
		}
		spxbot = NULL;
	}
	spxStats.spx_current_connections--;

	if (SpxDevicesOpen() == 0) {
		NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE,
			"NSPX: close: dequeueing WatchEmu %d",spxWatchEmuId));
		if (spxWatchEmuId) {
			untimeout(spxWatchEmuId);
			spxWatchEmuId = 0;
		}

		/* free memory on last close and if control Dev is/has been closed */
		if(spxbot == NULL) {
			if (spxSocketCount) {
				kmem_free(spxSocketTable,
					 (spxSocketCount * sizeof(spxSocketEntry_t)));
				spxSocketTable = NULL;
				spxSocketCount = 0;
			}
			if (spxMaxConnections) {
				kmem_free(spxConnectionTable,
					 (spxMaxConnections * sizeof(spxConnectionEntry_t)));
				spxConnectionTable = NULL;
				spxMaxConnections = 0;
			}
		}
	}

	/* null q pointer to denote device closed */
	q->q_ptr = NULL;
	OTHERQ(q)->q_ptr = NULL;

	NWSLOG((SPXID, 0, PNW_EXIT_ROUTINE, SL_TRACE, 
		"NSPX: close: EXIT spxclose procedure"));
	return( NTR_LEAVE(0));
}

/*
 * void SpxAckTimeOut(int index)
 *	SpxAckTimeout is scheduled by SpxSendAck when and acknowledgement is sent
 *	that opens the window after being closed. This routine will send a ping
 *	to the other endpoint forcing it to send an back acknowledgment.  This
 *	is needed for error recovery if the acknowledgement reopening the 
 *	window is lost.  The Ping will act as a ACK and will reopen the window.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxAckTimeOut(int index)
{
	register spxConnectionEntry_t *conEntryPtr;

	NTR_ENTER( 1, index, 0, 0, 0, 0);
	NWSLOG((SPXID, index, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: AckTimeOut: ENTER"));

	if (spxbot == NULL) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: AckTimeOut : link to IPX broken"));
		NTR_VLEAVE();
		return;
	}

	if (index >= spxMaxConnections) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: AckTimeOut : bad connection index %d ",index));
		NTR_VLEAVE();
		return;
	}

	conEntryPtr = &spxConnectionTable[index];
	conEntryPtr->aTimeOutId = 0;	  /* Prevent calls to untimeout */

	/* set timeout for 3 sec */
	conEntryPtr->aTimeOutId = itimeout(SpxAckTimeOut, 
		(void *)index, (3 * HZ), pltimeout);
	SpxPing(index);

	NWSLOG((SPXID, index, PNW_EXIT_ROUTINE, SL_TRACE,
		"NSPX: AckTimeOut: EXIT"));
	NTR_VLEAVE();
	return;
}

/*
 * void SpxPTimeOut(int index)
 *	If no answer from watchdog packet, try again or start Re-negotiation.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxPTimeOut(int index)
{
	register spxConnectionEntry_t *conEntryPtr;
	mblk_t *nmp;
	ipxMctl_t *spxMctlPtr;
	uint32 netwrk;

	NTR_ENTER( 1, index, 0, 0, 0, 0);

	if (spxbot == NULL) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: PingTimeOut : link to IPX broken"));
		NTR_VLEAVE();
		return;
	}

	if (index>=spxMaxConnections) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: PingTimeOut : bad connection index %d ",index));
		NTR_VLEAVE();
		return;
	}
	conEntryPtr = &spxConnectionTable[index];
	conEntryPtr->pTimeOutId = 0;	  /* Prevent calls to untimeout */

	NWSLOG((SPXID, index, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX:PingTimeOut : ENTER"));

	if (conEntryPtr->upperReadQueue == NULL ) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: PingTimeOut: connection closed"));
		NTR_VLEAVE();
		return;
	}

	if (conEntryPtr->pRetries >= conEntryPtr->tMaxRetries ) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: PingTimeOut: reached max retries, find new Route"));
		if ((nmp = allocb(sizeof(ipxMctl_t), BPRI_HI)) == NULL) {
			spxStats.spx_alloc_failures++; 
			NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
				"NSPX: PingTimeOut: cant alloc %d ",sizeof(ipxMctl_t)));
			conEntryPtr->allocRetryCount++; 
			goto Exit;
		}
		conEntryPtr->allocRetryCount = 0; 
		/* disable WR queue until Re-negotiation  is done */
		noenable(WR(conEntryPtr->upperReadQueue));
		conEntryPtr->disabled = TRUE;
		conEntryPtr->sessionFlags |= RE_NEGOTIATE;
		IPXCOPYNET(conEntryPtr->spxHdr.ipxHdr.dest.net, &netwrk);
		MTYPE(nmp) = M_CTL;
		nmp->b_wptr = nmp->b_rptr + sizeof(ipxMctl_t); 
		spxMctlPtr = (ipxMctl_t *)nmp->b_rptr;	
		spxMctlPtr->mctlblk.cmd = SPX_GET_IPX_MAX_SDU; 
		spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.network = netwrk; 
		spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.maxSDU = 0; 
		spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.conEntry = 
					(caddr_t)conEntryPtr; 
		putnext(spxbot,nmp);
		NTR_VLEAVE();
		return;
	}

	conEntryPtr->tTicksToWait += (conEntryPtr->tTicksToWait >> 1);
	 
	if ((conEntryPtr->tTicksToWait > conEntryPtr->maxTicksToWait ) ||
			(conEntryPtr->tTicksToWait == 0))
		conEntryPtr->tTicksToWait = conEntryPtr->maxTicksToWait;

	NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
		"NSPX: PingTimOut: ticks till next retry %d ",
			conEntryPtr->tTicksToWait));
	
	conEntryPtr->pRetries++;
	conEntryPtr->pTimeOutId = itimeout(SpxPTimeOut,
			(void *)index, conEntryPtr->tTicksToWait, pltimeout);

	SpxPing(index);

	Exit:
		NTR_VLEAVE();
		return;
}

/*
 * void SpxTTimeOut(int index)
 *	Re-transmit last packet, and schedule another retry.
 *	If 1/2 max retries try a watchdog packet.
 *	If max retries clse the connection.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxTTimeOut(int index)
{
	spxConnectionEntry_t	*conEntryPtr;
	mblk_t 					*spxCopy;
	mblk_t 					*nmp;
	mblk_t					*mp, *flushMp;
	struct T_ok_ack			*tOkAck;
	spxHdr_t 				*spxPacket; 
	ipxMctl_t				*spxMctlPtr;
	int 					lastMsg;
	uint32 					netwrk;

	NTR_ENTER( 1, index, 0, 0, 0, 0);

	if (spxbot == NULL) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: TxTimeOut : link to IPX broken"));
		NTR_VLEAVE();
		return;
	}

	if (index >= spxMaxConnections) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: TxTimeOut : bad connection index %d ",index));
		NTR_VLEAVE();
		return;
	}

	conEntryPtr = &spxConnectionTable[index];
	NWSLOG((SPXID, index, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: TxTimeOut: ENTER index= 0x%x",index));
	conEntryPtr->tTimeOutId = 0;	  /* Prevent calls to untimeout */


	NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
		"NSPX: TxTimeOut: retry %d on sequence %d",
			conEntryPtr->tRetries,
			conEntryPtr->window.local.retrySequence));

	if (conEntryPtr->upperReadQueue == NULL ) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: TxTimeOut: connection closed"));
		NTR_VLEAVE();
		return;
	}
	if (conEntryPtr->allocRetryCount >= spxMaxAllocRetries) {	
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: TTimeOut: reached max alloc retries %d",
			spxMaxAllocRetries));
		goto FreeAndFreeze;
	}
	/* If we reached max retries, we timed out on connect request or
	   packet didn't make it after re-negotiation. Disconnect now*/
	if (conEntryPtr->tRetries >= conEntryPtr->tMaxRetries ) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: TTimeOut: reached max retries %d, Disconnect",
			conEntryPtr->tMaxRetries));
		spxStats.spx_max_retries_abort++; 
		goto FreeAndFreeze;
	}

	/*
		We have reached max retries on DisConnect Request. Reset the 
		connection and send T_ok_ack up, the application is still waiting
		for it. */
	if ((conEntryPtr->tRetries >= conEntryPtr->tMaxRetries) &&
		(conEntryPtr->sessionFlags == WAIT_TERMINATE_ACK )) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
		 "NSPX: TTimeOut: reached max retries %d, on TERMINATE REQ Disconnect",
			conEntryPtr->tMaxRetries));

		conEntryPtr->sessionFlags &= ~WAIT_TERMINATE_ACK;
		NWSLOG((SPXID, index, PNW_ASSIGNMENT, SL_ERROR,
			"NSPX: TxTimeOut: EXIT "));
		SpxResetConnection(conEntryPtr);
 
		conEntryPtr->needAck = FALSE;
		conEntryPtr->state = TS_IDLE;
 
/* per tli spec we must flush before putting up */
		if ((flushMp = allocb(1, BPRI_MED)) == NULL ) {
			spxStats.spx_alloc_failures++; 
			NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
				"NSPX: TTimeOut: cant alloc msg size 1"));
			conEntryPtr->allocRetryCount++; 
			goto TryAgain;
		} else {
			*(flushMp->b_rptr) = FLUSHR | FLUSHW; /* flushFlags */
			MTYPE(flushMp) = M_FLUSH;
			flushMp->b_wptr = flushMp->b_rptr + 1;
		}
		if ((mp = allocb(sizeof(struct T_ok_ack), BPRI_MED)) == NULL) {
			spxStats.spx_alloc_failures++; 
			NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
				"NSPX: TTimeOut: cant alloc %d ",sizeof(struct T_ok_ack)));
			conEntryPtr->allocRetryCount++; 
			freemsg(flushMp);
			goto TryAgain;
		}

		tOkAck = (struct T_ok_ack *)mp->b_rptr;
		tOkAck->PRIM_type = T_OK_ACK;
		tOkAck->CORRECT_prim = T_DISCON_REQ;
		mp->b_wptr = mp->b_rptr + sizeof(struct T_ok_ack);
		MTYPE(mp) = M_PCPROTO;
		
		putnext(conEntryPtr->upperReadQueue,flushMp);
		putnext(conEntryPtr->upperReadQueue,mp);
	
		NTR_VLEAVE();
		return;
	}

	/* If we reached 1/2 max retries and in XFER state, try to find a new route
	   to endpoint and re-negotiate packet size. Only find new route
	   (re-negotiate) if we had a connection already*/

	if ((conEntryPtr->tRetries == (conEntryPtr->tMaxRetries/2)) &&
		((conEntryPtr->state == TS_DATA_XFER) ||
		(conEntryPtr->state == TS_WREQ_ORDREL))) {

		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: TxTimeOut: reached 1/2 max retries, find new Route"));
		if ((nmp = allocb(sizeof(ipxMctl_t), BPRI_HI)) == NULL) {
			spxStats.spx_alloc_failures++; 
			NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
				"NSPX: TxTimeOut: cant alloc %d ",sizeof(ipxMctl_t)));
			conEntryPtr->allocRetryCount++; 
			goto TryAgain;
		}
		conEntryPtr->tRetries++;
		conEntryPtr->allocRetryCount = 0; 
		/* disable WR queue until Re-negotiation  is done */
		noenable(WR(conEntryPtr->upperReadQueue));
		conEntryPtr->disabled = TRUE;
		conEntryPtr->sessionFlags |= RE_NEGOTIATE;
		IPXCOPYNET(conEntryPtr->spxHdr.ipxHdr.dest.net, &netwrk);
		MTYPE(nmp) = M_CTL;
		nmp->b_wptr = nmp->b_rptr + sizeof(ipxMctl_t); 
		spxMctlPtr = (ipxMctl_t *)nmp->b_rptr;	
		spxMctlPtr->mctlblk.cmd = SPX_GET_IPX_MAX_SDU; 
		spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.network = netwrk; 
		spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.maxSDU = 0; 
		spxMctlPtr->mctlblk.u_mctl.spxGetIpxMaxSDU.conEntry = 
					(caddr_t)conEntryPtr; 
		putnext(spxbot,nmp);
		NTR_VLEAVE();
		return;
	}
	conEntryPtr->tTicksToWait += (conEntryPtr->tTicksToWait >> 1);
	 
	if ((conEntryPtr->tTicksToWait > conEntryPtr->maxTicksToWait ) ||
			(conEntryPtr->tTicksToWait == 0))
		conEntryPtr->tTicksToWait = conEntryPtr->maxTicksToWait ;

	if (!canputnext(spxbot)) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: TxTimeOut: cant put down to q %Xh",spxbot));
		conEntryPtr->conStats.con_canput_fail++; 
		conEntryPtr->tRetries++;
		goto TryAgain;
	}

	lastMsg = (uint16)conEntryPtr->window.local.retrySequence % 
				MAX_SPX2_WINDOW_SIZE;
	if (conEntryPtr->window.unAckedMsgs[lastMsg] && 
				conEntryPtr->window.unAckedMsgs[lastMsg]->b_datap) {
		if ((spxCopy = dupmsg(conEntryPtr->window.unAckedMsgs[lastMsg]))
			 == NULL) {
			NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
				"NSPX: TxTimeOut: (dupmsg) cant alloc %d",
				msgdsize(conEntryPtr->window.unAckedMsgs[lastMsg])));
			spxStats.spx_alloc_failures++; 
			conEntryPtr->allocRetryCount++; 
			goto TryAgain;
		}
	} else {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
		"SpxTTimeOut: NO Data to Send- lastmsg =0x%x mp = 0x%x datap = 0x%x\n",
			lastMsg, conEntryPtr->window.unAckedMsgs[lastMsg],
			conEntryPtr->window.unAckedMsgs[lastMsg] ? 
			conEntryPtr->window.unAckedMsgs[lastMsg]->b_datap : 0));
		NTR_VLEAVE();
		return;
	}
	conEntryPtr->allocRetryCount = 0; 
	spxPacket = (spxHdr_t *)spxCopy->b_rptr; 
	spxPacket->connectionControl |= SPX_ACK_REQUESTED;
	/* insert current ACK and ALLOC Numbers */
	spxPacket->acknowledgeNumber = GETINT16(conEntryPtr->window.local.ack);  
	spxPacket->allocationNumber = GETINT16(conEntryPtr->window.local.alloc);  
	conEntryPtr->window.local.lastAdvertisedAlloc =
		conEntryPtr->window.local.alloc;  
	conEntryPtr->needAck = TRUE; 
	
	/* this is a duplicated (dupmsg) packet that is being re-sent, IPX 
	 * modified the checksum field on the original packet which changed 
	 * this checksum field also. If checksums are enabled put in the IPX 
	 * Trigger value so Ipx will generate a checksum.
	 */
	if ((conEntryPtr->ipxChecksum == TRUE) && 
		((conEntryPtr->state == TS_DATA_XFER) ||
		(conEntryPtr->state == TS_WREQ_ORDREL) ||
		(conEntryPtr->state == TS_WIND_ORDREL))) {
				spxPacket->ipxHdr.chksum = IPX_CHKSUM_TRIGGER;
	}
	spxStats.spx_send_packet_timeout++; 
	conEntryPtr->conStats.con_send_packet_timeout++; 
	conEntryPtr->tRetries++;

	NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
		"NSPX: TxTimeOut: ticks till next retry %d",
			conEntryPtr->tTicksToWait));

	drv_getparm(LBOLT, (void *)&conEntryPtr->beginTicks);
	conEntryPtr->tTimeOutId = itimeout(SpxTTimeOut,
		(void *)index, conEntryPtr->tTicksToWait, pltimeout);

	putnext(spxbot,spxCopy);
	NWSLOG((SPXID, index, PNW_ASSIGNMENT, SL_ERROR,
		"NSPX: TxTimeOut: EXIT Reschedule TTimeOut ID=%x",
		conEntryPtr->tTimeOutId));

	NTR_VLEAVE();
	return;
		
	TryAgain:

		conEntryPtr->tTimeOutId = itimeout(SpxTTimeOut,
			(void *)index, conEntryPtr->tTicksToWait, pltimeout);
		NWSLOG((SPXID, index, PNW_ASSIGNMENT, SL_ERROR,
			"NSPX: TxTimeOut: EXIT Reschedule TTimeOut ID=%x",
			conEntryPtr->tTimeOutId));
	
		NTR_VLEAVE();
		return;

	FreeAndFreeze:
		NWSLOG((SPXID, index, PNW_ERROR, SL_ERROR,
			"NSPX: TxTimeOut: transmit timeout connection failed"));
		SpxGenTDis(conEntryPtr, (long) TLI_SPX_CONNECTION_FAILED, FALSE);
		NTR_VLEAVE();
		return;
}

/*
 * void SpxNegTimeOut(int index)
 *	if no answer from Session Setup or Negotiation Req, close connection.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
SpxNegTimeOut(int index)
{
	register spxConnectionEntry_t *conEntryPtr;

	NTR_ENTER( 1, index, 0, 0, 0, 0);

	if (spxbot == NULL) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: NegTimeOut : link to IPX broken"));
		NTR_VLEAVE();
		return;
	}

	if (index >= spxMaxConnections) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: NegTimeOut : bad connection index %d ",index));
		NTR_VLEAVE();
		return;
	}

	conEntryPtr = &spxConnectionTable[index];
	conEntryPtr->nTimeOutId = 0;	  /* Prevent calls to untimeout */

	NWSLOG((SPXID, index, PNW_ENTER_ROUTINE, SL_TRACE,
		"NSPX: NegTimeOut: ENTER"));

	if (conEntryPtr->upperReadQueue == NULL ) {
		NWSLOG((SPXID, index, PNW_ERROR, SL_TRACE,
			"NSPX: NegTimeOut: connection closed"));
		NTR_VLEAVE();
		return;
	}

	NWSLOG((SPXID, index, PNW_ERROR, SL_ERROR,
		"NSPX: NegTimeOut: Negotiation timeout connection failed"));

	spxStats.spx_max_retries_abort++; 
	SpxGenTDis(conEntryPtr, (long) TLI_SPX_CONNECTION_FAILED, FALSE);
	NTR_VLEAVE();
	return;
}

#ifdef DEBUG 
/*
 * void dump_mp (mblk_t *m)
 *	Debug routine to dump an mblk_t to the console.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */
void
dump_mp (mblk_t *m)
{
	mblk_t  *tmp;
	u_char  *ptr;

	tmp = m;
	cmn_err(CE_NOTE,"MP: ");
	while (tmp) {
		for (ptr=(u_char *)tmp->b_rptr; ptr<(u_char *)tmp->b_wptr; ptr++)
			cmn_err(CE_CONT," %x", *ptr);
		cmn_err(CE_CONT," /");
		tmp = tmp->b_cont;
	}
	cmn_err(CE_CONT,"DONE\n");
}
#endif		

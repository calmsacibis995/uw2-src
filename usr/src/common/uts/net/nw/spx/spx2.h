/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spx/spx2.h	1.12"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_SPX_SPX2_H  /* wrapper symbol for kernel use */
#define _NET_NW_SPX_SPX2_H  /* subject to change without notice */

#ident	"$Id: spx2.h,v 1.11.4.1 1995/01/31 20:35:19 vtag Exp $"

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

/* Define SPX_INFO to default of spxinfo */
#ifdef SPX_INFO
#undef SPX_INFO
#endif

#define SPX_INFO nspxinfo

#ifdef _KERNEL_HEADERS
#include <net/nw/nwcommon.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <util/sysmacros.h>
#include <net/nw/ipx_app.h>
#include <net/nw/spx_app.h>
#include <net/nw/spx/spx_tune.h>
#include <net/nw/spxipx.h>
#else
#include "nwcommon.h"
#include "spx_opt.h"
#include <sys/tihdr.h>
#include <sys/tiuser.h>
#include <sys/sysmacros.h>
#include "sys/ipx_app.h"
#include "sys/spx_app.h"
#include "spx_tune.h"
#include "spxipx.h"
#endif /* _KERNEL_HEADERS */

#ifdef NTR_TRACING
#define NTR_ModMask     NTRM_spx
#endif

/* data transmit parameters */
/* max number of transmit retries until failure */ 

#define MAX_SPX_TRETRIES_COUNT	50 
#define MIN_SPX_TRETRIES_COUNT	3 

/* SPX2 Minimum Retry Delay
   The Minimum delay (in millisecond) between transmit Retries. 
   This parameter will be coverted to "ticks", the millisecond value 
   entered will be at least " 1 tick" which is UNIX system dependent. 
*/

#define MAX_SPX2_MIN_RETRY_DELAY 60000
#define MIN_SPX2_MIN_RETRY_DELAY 10

/* SPX2 Maximum Retry Delta */
/* The Maximum delta (in second) between transmit Retries. */

#define MAX_SPX2_MAX_RETRY_DELTA 60
#define MIN_SPX2_MAX_RETRY_DELTA 1

/*
	SPX2_WINDOW_SIZE is the maximum number of packets
	that a connection can receive before an acknowledge.
	This value is used to calculate the allocation number.
*/
#define MAX_SPX2_WINDOW_SIZE	16
#define MIN_SPX2_WINDOW_SIZE	1

/*
	 SPX_MIN/MAX_CONN are the minimum and maximum values for the 
	 "spx_max_connections" parameter that is passed down by the 
	 SPX_SET_PARAM ioctl. 
*/
#define SPX_MIN_CONN 	5
#define SPX_MAX_CONN	1024

/*
	 SPX_MIN/MAX_SOCKETS are the minimum and maximum values for the 
	 "spx_max_sockets" parameter that is passed down by the 
	 SPX_SET_PARAM ioctl. 
*/
#define SPX_MIN_SOCKETS 	5
#define SPX_MAX_SOCKETS		200 

#define SPX_HI_WATER_MARK 20000
#define SPX_LOW_WATER_MARK 15000

#define SPX_SAVE_SEND_HEADER_FLAG	0x1

#define SPX_CHECKSUM				0xFFFF
#define SPX_PACKET_HEADER_SIZE 			42 
#define SPXII_PACKET_HEADER_SIZE 		44 
#define SPX_CON_REQUEST_CONID		0xFFFF
#define SPX_PACKET_TYPE 			5

#define SPX_SYSTEM_PACKET 			0x80
#define SPX_ACK_REQUESTED 			0x40
#define SPX_EOF_BIT					0x10	
#define SPX_SPXII					0x08	
#define SPX_NEG						0x04	

#define SPX_TERMINATE_ACK			0xFF
#define SPX_TERMINATE_REQUEST 		0xFE
#define SPX_ORDERLY_REL_REQ 		0xFD

#define SPX_TSDU_SIZE				-1
#define SPX_ETSDU_SIZE				-2
#define SPX_CDATA_SIZE				-2
#define SPX_DDATA_SIZE				-2
#define SPX_INIT_RUN_FLAG			0x0A0A0A0A
#define SPX_DEFAULT_PACKET_SIZE		576


/* SpxII Session Flags */
#define NO_NEGOTIATE				0x8000
#define SESS_SETUP					0x4000
#define WAIT_NEGOTIATE_ACK			0x2000
#define WAIT_NEGOTIATE_REQ			0x1000
#define RE_NEGOTIATE				0x0800
#define WAIT_TERMINATE_ACK			0x0400


/* SpxII Negotiation Options */
#define NUMBER_OF_OPTIONS	02		/* Only 2 option for know */

#define OPT_SIZE_MASK	0x03		/* Size Mask (xxxx xx11) */
#define EXT_ID_MASK		0xFC		/* Size Mask (1111 1100) */

#define U8		00 					/* Size uint8 (xxxx xx00) */
#define U16		01 					/* Size uint16 (xxxx xx01) */
#define U32		02 					/* Size uint32 (xxxx xx10) */
#define UEXT	03 					/* Size Extended (xxxx xx11) */
/* with extended ID  */
#define EID_U8		0xFC 			/* Extended ID, Size uint8 (1111 1100) */
#define EID_U16		0xFD 			/* Extended ID, Size uint16 (1111 1101) */
#define EID_U32		0xFE 			/* Extended ID, Size uint32 (1111 1110) */
#define EID_UEXT	0xFF 			/* Extended ID, Size Extended (1111 1111) */

#define SPXII_RTT	(00 | U32)	/*Type 0 (0000 00XX)Size uint32 (xxxx xxx10)*/
								/*Round Trip Time */

#define IPX_CHKSM	(04 | U8)	/*Type 1 (0000 01XX)Size uint8 (xxxx xxx00)*/
								/* Ipx Check Sums */



/* socket allocation stuff */
#define	SPX_MIN_EPHEMERAL_SOCKET	0x4000
#define	SPX_MAX_EPHEMERAL_SOCKET	0x5000
#define SPX_EPHEMERAL_SOCKET_RANGE	(SPX_MAX_EPHEMERAL_SOCKET - \
										SPX_MIN_EPHEMERAL_SOCKET)

/* global variable for spx  */
extern uint32 				spxMaxAllocRetries;
extern uint32 				spxMaxListensPerSocket;
extern uint32 				spxMaxTRetries;
extern uint32 				spxMaxRRetries;
extern uint32 				spxMinTRetryTicks;
extern uint32 				spxMaxTRetryTicks;
extern uint32 				spxWatchEmuInterval;
extern uint32 				spxMaxReadRetryTicks;
extern uint32				spxDefaultRWindow;
extern uint16 				spxWindowSize;
extern uint16 				spxIpxChecksum;
spxStats_t					spxStats;
extern ipxMctl_t			spxIpxMctl;
extern int					strmsgsz;

typedef struct Opt8 {
    uint8  TypeSize;
    uint8 Value;
    } Opt8_t;

typedef struct Opt16 {
    uint8  TypeSize;
    uint8 Value[2];
    } Opt16_t;

typedef struct Opt32 {
    uint8  TypeSize;
    uint8 Value[4];
    } Opt32_t;

typedef struct negoHdr {
    uint16  numberOfOptions;
    Opt32_t RttOpt;
    Opt8_t ChksmOpt;
    } negoHdr_t;

typedef struct spxUnitHdr {
	ipxAddr_t addr;	
	uint16 connectionId;
	uint16 allocationNumber;
} spxUnitHdr_t;

typedef struct listenUnit {
	spxUnitHdr_t spxInfo;
	uint8 acked;
} spxListenUnit_t;

typedef struct socketEntry {
	uint16 socketNumber;
	uint16 listenConnectionId; 
	uint16 nextToConnect;
	uint16 postedListens;
	spxListenUnit_t listens[SPX_MAX_LISTENS_PER_SOCKET];
} spxSocketEntry_t;
/*
 * Private data structure for each open stream to the spx driver
 */

typedef struct {
    uint16 ack;
    uint16 alloc;
} spxRemoteWindow_t;

typedef struct {
    uint16 sequence;
    uint16 ack;
    uint16 alloc;
    uint16 lastAdvertisedAlloc;
    uint16 lastReNegReq_AckNum;
    uint16 retrySequence;
    uint16 windowClosed;
} spxLocalWindow_t;

typedef struct {
    mblk_t *unAckedMsgs[MAX_SPX2_WINDOW_SIZE];
    mblk_t *outOfSeqPackets[MAX_SPX2_WINDOW_SIZE];
    spxLocalWindow_t	local;
    spxRemoteWindow_t	remote;
} spxWindow_t;

typedef struct connectionEntry {
    spxHdr_t spxHdr;
    queue_t *upperReadQueue;
    mblk_t *waitOnDataMp;
    uint8 needAck;
    long state;
    long spx2Options;
    uint16 tRetries;
    uint16 pRetries;
    uint16 tMaxRetries;		/* spxo_max_retries, 	 spxIIRetryCount */
    uint16 spxWinSize;      /*						 spxIILocalWindowSize */
    uint32 minTicksToWait;	/* spxo_min_retry_delay, spxIIMinimumRetryDelay */
    uint32 maxTicksToWait;	/*					     spxIIMaximumRetryDelta */
    uint32 tTicksToWait;
    uint32 beginTicks;
    uint32 endTicks;
    uint32 lastTickDiff;
    uint32 remoteActiveTicks;
    uint32 allocRetryCount;
    uint32 specialFlags;
    int tTimeOutId;
	spxWindow_t	window;
    uint8 protocol;         /*  SPX or SPXII for this connection */
    uint8 disabled;         /*  spx write queue is disabled */
    uint32 maxSDU;          /* Maxium driver packet size from IPX */
    uint16 sessionFlags;
    uint16 sizeNegoRetry;
    uint32 maxRPacketSize;  /* maxium Recieve packet size after negot*/
    uint32 maxTPacketSize;  /* maxium transmit packet size after negot*/
    uint32 ticksToNet;
    int nTimeOutId;			/* negotiation timeout Id */
    int aTimeOutId;			/* Acknowledge timeout Id */
    int pTimeOutId;			/* Ping timeout Id */
    queue_t *responseQueue;
    uint16 listenSocket;
    uint16 acceptSocket;
    uint16 goingUpCount;
    uint16 flowControl;
    uint16 ipxChecksum;
    uint16 ordRelReqSent;
    uint16 ordSeqNumber;
	spxConStats_t conStats;	/* statistics for each connection  see spx_app.h*/
} spxConnectionEntry_t;

/*
 * Convert these MACRO to use drv_hztousec and drv_usectohz
 */
#define MSEC2TCKS(val) (((val) * HZ/1000) + 1)
#define TCKS2MSEC(val) ((val) * 1000/HZ)

/*
** Defines ands Macros for pseudo-random Connection IDs
*/

/*
** Mask to convert conId to connectionTable Index, 
** allow upto 1023 connections
*/
#define CONIDTOINDEX   0x3ff

#define SpxIDtoEntry(conId) \
		&spxConnectionTable[(uint16)(conId & CONIDTOINDEX)]

/*
**	Prototypes
*/

void		SpxGenTDis(spxConnectionEntry_t *, long, int);
long		SpxSendAck(uint16, int, uint16);
void		SpxSendDisconnect(spxConnectionEntry_t *, uint8);
void		SpxSendNegSessPkt(spxConnectionEntry_t *);
long		SpxTranData(spxConnectionEntry_t *, mblk_t *, uint8, uint8);
void		SpxTUnbindReq(queue_t *, uint8);
void		SpxTAddrReq(queue_t *);
void		SpxTConnReqCont(spxConnectionEntry_t *, mblk_t *);
void		SpxPing(uint16);
void		SpxResetConnection(spxConnectionEntry_t *);
void		SpxAckTimeOut(int index);
void		SpxTTimeOut(int index);
void		SpxNegTimeOut(int index);
void		SpxTBindReqCont(queue_t *, uint16, uint16, int, long);
void		SpxGenTErrorAck(queue_t *, long, long, long);
mblk_t *	SpxSetUpPacket(int i);
void		SpxRelIpxSocket(uint16);


/*
 * STREAMS Declarations:
 */
int         nspxursrv(queue_t *q);
int         nspxlrput(queue_t *q, mblk_t *mp);

/*
 * GLobal Variables
 */

extern  uint32  IpxInternalNet;
extern  uint8   IpxInternalNode[];

extern  queue_t                 *spxbot;
extern  int                     spxMinorDev;
extern  uint                    spxMaxConnections;
extern  spxConnectionEntry_t    *spxConnectionTable;
extern  uint                    spxSocketCount;
extern  spxSocketEntry_t        *spxSocketTable;

#endif /* _NET_NW_SPX_SPX2_H */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
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

#ifndef _NET_NW_LIPMX_APP_H  /* wrapper symbol for kernel use */
#define _NET_NW_LIPMX_APP_H  /* subject to change without notice */

#ident	"@(#)kern:net/nw/lipmx_app.h	1.13"
#ident	"$Id: lipmx_app.h,v 1.10.2.1.2.1 1994/12/20 22:12:00 vtag Exp $"

/*
 * Copyright 1991, 1992 Novell, Inc. 
 * All Rights Reserved.
 *
 * This work is subject to U.S. and International copyright laws and
 * treaties.  No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 *
 */

#ifndef IPX_DIAGNOSTIC_SOCKET
#ifdef _KERNEL_HEADERS
#include <net/nw/ipx_app.h>
#else
#include <sys/ipx_app.h>
#endif /* _KERNEL_HEADERS */
#endif

/*
**	The following IOCTL's are present with all IPX drivers.  
**
**	Ioctl functions
*/
#define IPX_GET_NODE_ADDR			( ( 'i' << 8 ) | 5 )
#define IPX_GET_NET					( ( 'i' << 8 ) | 10 )
#define IPX_SET_LAN_INFO			( ( 'i' << 8 ) | 11 )
#define IPX_GET_LAN_INFO			( ( 'i' << 8 ) | 12 )
#define IPX_GET_CONFIGURED_LANS 	( ( 'i' << 8 ) | 17 )
#define IPX_SET_CONFIGURED_LANS 	( ( 'i' << 8 ) | 29 )
#define IPX_SET_SAPQ				( ( 'i' << 8 ) | 30 )
#define IPX_STATS					( ( 'i' << 8 ) | 31 )

/*
**	IPX_GET_NET - Returns 4 byte IPX net address for the internal lan
*/
typedef struct {
	ipxNet_t	myNetAddress;		/* In Net Order */
} IpxNetAddr_t;

/*
**	IPX_GET_NODE_ADDR - Returns 6 byte IPX node address for the internal lan
*/
typedef struct {
	ipxNode_t	myNodeAddress;	/* In Net Order */
} IpxNodeAddr_t;

/*
**	IPX_SET_LAN_INFO - pass net, mux, dlInfo data, and RIP/SAP data to LIPMX,
**		no data returned.
**	IPX_GET_LAN_INFO - get lanstate, mux, dlInfo, and RIP/SAP
*/

typedef struct dlInfo_s {
	uint32 SDU_max;				/* Max Service Data Unit */
	uint32 SDU_min;				/* Min Service Data Unit */
	uint32 ADDR_length;			/* Length of datalink physical addr */
	uint32 physAddrOffset;		/* Offset of datalink physical addr */
	int32 SAP_length;			/* Service Access Point Length */
	uint32 SAP;					/* Service Access Point */
	uint8	dlAddr[50];			/* Datalink Address */
} dlInfo_t;

#define	RECV_RIPS			0x01
#define	SEND_RIPS			0x02
#define	DO_RIP				(RECV_RIPS | SEND_RIPS)	/* 0x03 */
#define	RIP_UPDATES_ONLY	0x04
typedef struct {
	uint16		bcastInterval;	/* #x(30 sec) between broadcasts */
	uint16		ageOutIntervals;	/* # of bcastIntervals to age out */
	uint32		maxPktSize;		/* maximum # of bytes in packet */
	uint16		interPktGap;	/* interpacket gap in millisec */
	uint16		actions;		/* RCV_RIPS, SEND, RIP_UPDATES_ONLY */
} RIPperLanInfo_t;

#define	RECV_SAPS			0x01
#define	SEND_SAPS			0x02
#define	DO_SAP				(RECV_SAPS | SEND_SAPS)	/* 0x03 */
#define	SAP_UPDATES_ONLY	0x04
#define	SAP_REPLY_GNS		0x08
typedef struct {
	uint16		bcastInterval;	/* #x(30 sec) between broadcasts */
	uint16		ageOutIntervals;	/* # of sapBcstIntvs to age out */
	uint32		maxPktSize;		/* maximum # of bytes in packet */
	uint16		interPktGap;	/* interpacket gap in millisec */
	uint16		actions;		/* RCV, SEND, UPDATES_ONLY, REPLY_GNS */
} SAPperLanInfo_t;

typedef struct {
	uint32		lanSpeed;		/* LAN speed in Kb/sec */
	RIPperLanInfo_t	rip;
	SAPperLanInfo_t	sap;
} ripSapInfo_t;

typedef struct lanInfo {
	uint32		lan;			/* Fill in for GET */
	uint32		state;			/* This value returned by GET */
	uint32		streamError;	/* This value returned by GET */
	uint32		network;		/* Fill in for SET, returned by GET mach order */
	uint32		muxId;			/* Fill in for SET, returned by GET */
	uint8		nodeAddress[6];	/* Ignored by SET, dlInfo value used */
								/*	returned by GET */
	dlInfo_t	dlInfo;			/* DataLink Layer info, supplied by the user, */
								/*	returned by GET */
	ripSapInfo_t ripSapInfo;	/* RIP and SAP lan info */
								/* Fill in for SET, returned by GET */
} lanInfo_t;

/*
 * state values in lanInfo_t - network link states
 */
#define IPX_UNBOUND 0
#define IPX_LINKED 1
#define IPX_NET_SET 2
#define IPX_IDLE 3

/*
**	IPX_SET_CONFIGURED_LANS - Initialization, set number of LANS
**		A slot must be reserved for the internal lan whether or not
**		it is configured.  No data is returned.
**	IPX_GET_CONFIGURED_LANS - Get configured LANS, returns
**		IpxConfiguredLans_t
*/
typedef struct {
	uint32	lans;				/* Maximum number of lans */
	uint16	maxHops;			/* Maximum Router hops for this SERVER */
} IpxConfiguredLans_t;

/*
**	IPX_SET_SAPQ - Binds a file descriptor to receive SAP packets
**		This ioctl is sent only if IPX is not in the stream (no internal net).
**		If IPX is in the stream, SAP packets go to IPX.
**		No data is required or returned for IPX_SET_SAPQ.
*/

#define IPX_MIN_CONFIGURED_LANS 2
#define IPX_MAX_CONFIGURED_LANS 1024

/*
 * information about the lan adapter that ipx is
 * hooked to -  compare DL_info_ack.
 */
typedef struct ipxAdapterInfo_s {
	uint32 PRIM_type; 
	uint32 SDU_max;
	uint32 SDU_min;
	uint32 ADDR_length;
	int32 SUBNET_type;
	int32 SERV_class;
	int32 CURRENT_state;
	int32 SAP_length;
	uint32 SAP;
} ipxAdapterInfo_t;

/*
**	Constants defining type of IPX loaded, see lipmxInfo_t->IpxType
*/
#define IPX_LAN_ROUTER_SOCKET_ROUTER
/*
**	Statistics Structure
*/
typedef struct {
	uint8  LipmxMajorVersion;	/* Major Version */ 
	uint8  LipmxMinorVersion;	/* Minor Version */ 
	char   LipmxRevision[2];	/* Revision */ 
	uint32 StartTime;			/* Time driver started */
	uint8  Reserved[2];			/* Reserved, don't use this */
	uint8  IpxInternalNet;		/* 1 = Internal Network exists */
	uint8  IpxSocketMux;		/* 1 = Socket mux exists, 0=No socket mux */
	uint32 TrimPacket;       	/* Data pkts, padding trimmed */
	uint32 InProtoSize; 	   	/* In pkts proto size bad, driver problem?, dropped */
	uint32 InBadDLPItype;     	/* In pkts unknown DLPI type, dropped */
	uint32 InTotal;          	/* In total data pkts */
	uint32 InBadLength;        	/* In data pkts with bad length, dropped */
	uint32 InCoalesced;        	/* In data pkts, multple blks coalesced */
	uint32 InPropagation;		/* In data type 0x14 - propagation pkts */
	uint32 InNoPropagate;		/* In data type 0x14 - pkts not propagated */
	uint32 InDriverEcho;    	/* In broadcast pkts, echoed by driver */
	uint32 InRip;            	/* In total rip pkts */
	uint32 InRipRouted;			/* In rip pkts processed & routed to int net */
	uint32 InRipDropped;		/* In rip pkts processed and dropped */
	uint32 InSap;            	/* In total sap pkts */
	uint32 InSapBad;	       	/* In sap pkts, invalid pkt, dropped */
	uint32 InSapIpx;	       	/* In sap pkts, sent to ipx */
	uint32 InSapNoIpxToSapd;   	/* In sap pkts, no ipx, sent to sapd */
	uint32 InSapNoIpxDrop;     	/* In sap pkts, no ipx, no sapd, dropped */
	uint32 InForward; 		   	/* In data pkts dest not on net, fwd to router */
	uint32 InInternalNet;    	/* In data pkts routed to internal net */
	uint32 InDiag;           	/* In total diag pkts */
	uint32 InDiagInternal;		/* In diag pkts, routed to internal net */
	uint32 InDiagNIC;          	/* In total diag pkts addressed to NIC */
	uint32 InDiagIpx;        	/* In diag pkts to NIC, routed to ipx */
	uint32 InDiagNoIpx;      	/* In diag pkts to NIC, no ipx, lipmx responded */
	uint32 InNICDropped;     	/* In data pkts sent to NIC, dropped */
	uint32 InRoute;          	/* In data pkts dest on net, routed */
	uint32 InBroadcast;      	/* In total broadcast data packets */
	uint32 InBroadcastInternal;	/* In total pkts bcast to intrnal net, routed */
	uint32 InBroadcastNIC;		/* In total pkts bcast to NIC */
	uint32 InBroadcastDiag;  	/* In total diag bcasts to NIC */
	uint32 InBroadcastDiagFwd;	/* In diag bcasts to NIC forward to my lans */
	uint32 InBroadcastDiagRoute;/* In diag bcasts to NIC, routed to ipx */
	uint32 InBroadcastDiagResp;	/* In diag bcasts to NIC lipmx responds */
	uint32 InBroadcastDropped; 	/* NOT USED In bcasts to NIC, dropped */
	uint32 InBroadcastRoute; 	/* In bcasts to local net, routed */
	uint32 OutTotalStream;	   	/* Out total data pkts from upstream */
	uint32 OutTotal;		   	/* Out total data pkts from all sources */
	uint32 OutPropagation;		/* Out pkts, type 0x14, propagation pkts */
	uint32 OutSameSocket;		/* Out pkts, to my net, src=dest, dropped */
	uint32 OutFillInDest;		/* Out pkts, fill in destination net/node */
	uint32 OutInternal;			/* Out pkts, routed to internal net */
	uint32 OutBadLan;			/* Out pkts, router retd bad lan, dropped */
	uint32 OutBadSize;			/* Out pkts, size > LAN SDU max, dropped*/
	uint32 OutNoLan;			/* Out pkts, lan no longer connected, dropped */
	uint32 OutPaced;			/* Out pkts, put on paced packet list */
	uint32 OutSent;				/* Out pkts, sent to connected lan */
	uint32 OutQueued;			/* Out pkts, queued to connected lan */
	uint32 Ioctl;				/* Ioctl - total */
	uint32 IoctlSetLans;		/* Ioctl - Set Configured Lans */
	uint32 IoctlGetLans;		/* Ioctl - Get Configured Lans */
	uint32 IoctlSetSapQ;		/* Ioctl - Set SAP Queue */
	uint32 IoctlSetLanInfo;		/* Ioctl - Set Lan Info */
	uint32 IoctlGetLanInfo;		/* Ioctl - Get Lan Info */
	uint32 IoctlGetNetAddr;		/* Ioctl - Get Net Address */
	uint32 IoctlGetNodeAddr;	/* Ioctl - Get Node Address */
	uint32 IoctlGetStats;		/* Ioctl - Get Statistics */
	uint32 IoctlLink;			/* Ioctl - Link */
	uint32 IoctlUnlink;			/* Ioctl - Unlink */
	uint32 IoctlUnknown;		/* Ioctl - Unknown Ioctl */
} IpxLanStats_t;

#endif /* _NET_NW_LIPMX_APP_H */

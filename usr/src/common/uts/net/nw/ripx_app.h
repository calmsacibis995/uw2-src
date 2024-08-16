/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ripx_app.h	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_RIPX_APP_H  /* wrapper symbol for kernel use */
#define _NET_NW_RIPX_APP_H  /* subject to change without notice */

#ident	"$Id: ripx_app.h,v 1.5 1994/05/10 15:06:09 meb Exp $"

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

#ifdef _KERNEL_HEADERS
#include <util/types.h>
#include <net/nw/nwtdr.h>
#include <net/nw/ipx_app.h>
#else
#include <sys/types.h>
#include <sys/nwtdr.h>
#include <sys/ipx_app.h>
#endif /* _KERNEL_HEADERS */

#define RIP_SOCKET	0x453

/*
**	Rip Ioctl functons
*/
#define RIPX_SET_SAPQ					( ( 'r' << 8 ) | 6 )
#define RIPX_RESET_ROUTER				( ( 'r' << 8 ) | 7 )
#define RIPX_GET_ROUTER_TABLE			( ( 'r' << 8 ) | 8 )
#define RIPX_DOWN_ROUTER 				( ( 'r' << 8 ) | 13 )
#define RIPX_GET_NET_INFO 				( ( 'r' << 8 ) | 14 )
#define RIPX_CHECK_SAP_SOURCE 			( ( 'r' << 8 ) | 15 )
#define RIPX_DUMP_HASH_TABLE 			( ( 'r' << 8 ) | 16 )
#define RIPX_START_ROUTER	 			( ( 'r' << 8 ) | 17 )
#define RIPX_INITIALIZE					( ( 'r' << 8 ) | 18 )
#define RIPX_GET_ROUTE_TABLE_HASH_SIZE 	( ( 'r' << 8 ) | 19 )
#define RIPX_GET_HASH_TABLE_STATS 		( ( 'r' << 8 ) | 30 )
#define RIPX_STATS			 			( ( 'r' << 8 ) | 31 )
#define RIPX_SET_ROUTER_TYPE 			( ( 'r' << 8 ) | 32 )

/*
**	RIPX_RESET_ROUTER - Clear all routing tables, restart with no routing info
**		Requires no data
*/

/*
** RIPX_GET_ROUTER_TABLE forms ROUTE_TABLE_SIZE message blocks
** fills each with complete routeInfo blocks, and sends them up stream
** as data until the all routes have been sent upstream to the calling
** process.  It then returns from the ioctl with no errors.
**
** If a failure occurs, the functon returns a -1 and the ioctl fails.
** If data blocks were sent, the last routeInfo entry will be have
** endOfTable set to TRUE, indicating end of data sent.
**
** The calling process receives the ioctl status, and if the ioctl
** was successful, retrieves the data  with the read or getmsg system calls.
*/

typedef struct routeInfo {
	uint32		net;					/* Network Id in net order */
	uint32		connectedLan;			/* Index of the lan for this route */
	uint16		hops;					/* Number of hops */
	uint16		time;					/* Route time */
	uint16		endOfTable;				/* TRUE if last route, else FALSE */
	uint8		node[IPX_NODE_SIZE];	/* Ipx Node of route*/
} routeInfo_t;

#define ROUTE_INFO_SIZE sizeof(routeInfo_t)
#define ROUTE_TABLE_SIZE (sizeof(routeInfo_t)*25)

/*
** RIPX_DOWN_ROUTER sends a broadcast to every known network
** informing them that this router is going down.
** It does not clear the routing tables.
**		Requires no data.
*/

/*
** RIPX_GET_NET_INFO returns information about the network specified
** by netIDNumber
*/
typedef struct netInfo {
	uint32				netIDNumber;	/* Specified by user in net order */
	uint16				timeToNet;		/* Returned by router in mach order */
	uint8				hopsToNet;		/* Returned by router */
	uint8				netStatus;		/* Returned by router */
	int32				lanIndex;		/* Index to net's lan or -1 if none */
} netInfo_t;

/* Bits for netStatus */
#define NET_LOCAL_BIT		0x01	/* net connected to my lan card */
#define NET_STAR_BIT		0x02	/* star lan */
#define NET_RELIABLE_BIT	0x04	/* slow or WAN link */
#define NET_WAN_BIT			0x08	/* slow or WAN link */
#define NET_ALIVE_BIT		0x40	/* not used */

/*
**  RIPX_CHECK_SAP_SOURCE validates the sap source indicated using information
** passed in the checkSapSource_t structure.  
*/
typedef struct checkSapSource_s {
	uint8 serverAddress[IPX_ADDR_SIZE];		/* Specified by user in net order */
	uint8 reporterAddress[IPX_NODE_SIZE];	/* Specified by user in net order */
	uint16 hops;							/* Specified by user in net order */
	uint32 connectedLan;					/* Specified by user in mach order */
	int result;						 /* returned by router */
									/* -1 not a good source  0 good source */
} checkSapSource_t;

/*
** RIPX_DUMP_HASH_TABLE returns an array of integer counters.  There is one
** entry in the array for each hash table slot.  The number of
** routes hashed on each slot is returned in the array.  You can find
** the size of the hash table by the RIPX_GET_ROUTE_TABLE_HASH_SIZE ioctl.
**
** Struct for RIPX_GET_ROUTE_TABLE_HASH_SIZE
*/
typedef struct {
	uint16	size;
} HashTableSize_t;

/*
** RIPX_INITIALIZE sets the size of the RIP Router hash
** table.  The value passed will be rounded up to a power of 2.
** RIPX_INITIALIZE returns the actual size of the hash table
** in the size variable..
** In addition, RIPX_INITIALIZE passes to RIPX the configured
** value for MAXIMUM HOPS and type of ROUTER (FULL, CLIENT, ...).
*/

/* bit definitions for router types. */
#define		FULL_ROUTER		0	/* full server router */
#define		CLIENT_ROUTER	1	/* client router, does not send any packets
								 * after initialization */

typedef struct {
	uint16	size;
	int8	hops;
	int8	type;
} Initialize_t;

/*
** RIPX_GET_HASH_TABLE_STATS returns a hashStats_t structure
*/

/* the Hash table used by the Routing Information Table (RIT)
 * uses simple chaining on a simple "mod" hash function (see
 * Sedgewick ISBN 0-201-06673-4 pp.234-236). Since the size of
 * the hash array (# keys) is configurable on-the-fly, these 
 * statistics can help give meaningful info on hash performance.  
 * 
 * Average length of of list examined for: 
 *	unsuccessful search 
 * 		= links/keys.
 * 	successful search
 * 		= posXcountSum/links.
 */
typedef struct	hashStats {
	uint16	keys;		/* # hash "buckets" */

	/* These are calculated via a table scan.
	 */
	uint16	blankKeys;						/* # unpopulated buckets */
	uint32	links;							/* # of table entries (nets) */
	uint32	posXcountSum;					/*
					* SUM(positionCount * #chainsHavingPositionX).
					* Divide this by "links" to get the successful search
					* average time (Sedgewick, p. 235).
					*/
	/*
	**These are maintained, actual values.
	*/
	uint32	accesses;
	uint32	collisions;
	uint32	failedFinds;
} hashStats_t;

/*
**	RIPX_INFO returns router statistics
*/
typedef struct RouterInfo {
	 uint8	RipxMajorVersion;		/* Major Version */ 
	 uint8	RipxMinorVersion;		/* Minor Version */ 
	 char	RipxRevision[2];		/* Revision */ 
	 time_t StartTime;				/* Time driver started */
	 uint32 ReceivedPackets;		/* Number of router pkts received */
	 uint32 ReceivedNoLanKey;		/* Number router pkts w/NULL lan key */
	 uint32 ReceivedBadLength;		/* Number of bad length router pkts */
	 uint32 ReceivedCoalesced;		/* Number of router pkts coalesced */
	 uint32 ReceivedNoCoalesce;		/* Number of router pkts w/coalesce fail */
	 uint32 ReceivedRequestPackets;	/* Number of router request packets */
	 uint32 ReceivedResponsePackets;/* Number of router response packets */
	 uint32 ReceivedUnknownRequest;	/* Number of router unknown request */
	 uint32 SentRouteRequests;		/* Number of router pkts sent */
	 uint32 SentRequestPackets;		/* Number of router request packets sent */
	 uint32 SentResponsePackets;	/* Number of router request packets sent */
	 uint32 SentAllocFailed;		/* Number of alloc of send pkt failures */
	 uint32 SentBadDestination;		/* Number of sent pkts, bad destinaton */
	 uint32 SentLan0Dropped;		/* Number of sent pkts, lan0 dropped */
	 uint32 SentLan0Routed;			/* Number of sent pkts, lan0 sent */
	 uint32 RipxIoctlSetSapQ;		/* Number ioctl RIPX_SET_SAPQ */
	 uint32 RipxIoctlInitialize;	/* Number ioctl INITIALIZE */
	 uint32 RipxIoctlGetHashSize;	/* Number ioctl GET_ROUTE_TABLE_HASH_SIZE */
	 uint32 RipxIoctlGetHashStats;	/* Number ioctl GET_HASH_TABLE_STATS */
	 uint32 RipxIoctlDumpHashTable;	/* Number ioctl GET_DUMP_HASH_TABLE */
	 uint32 RipxIoctlGetRouterTable;/* Number ioctl GET_ROUTER_TABLE */
	 uint32 RipxIoctlGetNetInfo;	/* Number ioctl GET_NET_INFO */
	 uint32 RipxIoctlCheckSapSource;/* Number ioctl CHECK_SAP_SOURCE */
	 uint32 RipxIoctlResetRouter;	/* Number ioctl RESET_ROUTER */
	 uint32 RipxIoctlDownRouter;	/* Number ioctl DOWN_ROUTER */
	 uint32 RipxIoctlStats;			/* Number ioctl RIPX_STATS */
	 uint32 RipxIoctlUnknown;		/* Number ioctl Unknown */
	hashStats_t	RITstats;
} RouterInfo_t;
#endif /* _NET_NW_RIPX_APP_H */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:api/ipx_app.h	1.1"
#ident	"$Header: $"

/*
 * Copyright 1989, 1991 Novell, Inc. All Rights Reserved.
 *
 * THIS WORK IS SUBJECT TO U.S. AND INTERNATIONAL COPYRIGHT LAWS AND
 * TREATIES.  NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 *
 *    include/api/ipx_app.h 1.2 (Novell) 7/30/91
 */

/*
 *   The parts of IPX exposed to the application programmer.
 */

#define IPX_SET_SAP 			( ( 'i' << 8 ) | 1 )
#define IPX_SET_SOCKET 			( ( 'i' << 8 ) | 2 )
#define IPX_SET_NET 			( ( 'i' << 8 ) | 3 )
#define IPX_SET_DEBUG_LEVEL		( ( 'i' << 8 ) | 4 )
#define IPX_GET_NODE_ADDR		( ( 'i' << 8 ) | 5 )
#define IPX_UNSET_SAP			( ( 'i' << 8 ) | 6 )
#define IPX_RESET_ROUTER		( ( 'i' << 8 ) | 7 )
#define IPX_GET_ROUTER_TABLE	( ( 'i' << 8 ) | 8 )
#define IPX_SET_SPX 			( ( 'i' << 8 ) | 9 )

#define IPX_HDR_SIZE		30
#define IPX_ADDR_SIZE		12
#define IPX_CHKSUM			0xFFFF
#define IPX_NET_SIZE		4
#define IPX_NODE_SIZE		6
#define IPX_SOCK_SIZE		2
#define IPXID 				362

/*
 * Structure defining an ipx address
 */
typedef struct ipxAddress {
	uint8  	net[IPX_NET_SIZE];		/* ipx network address */
	uint8 	node[IPX_NODE_SIZE];	/* ipx node address */
	uint8 	sock[IPX_SOCK_SIZE];	/* ipx socket */
} ipxAddr_t, tranAddr_t;

typedef struct ipxNet {
	uint8 net[IPX_NET_SIZE];	/* ipx network address */
} ipxNet_t;

typedef struct ipxNode {
	uint8 node[IPX_NODE_SIZE];	/* ipx node address */
} ipxNode_t;

typedef struct ipxSock {
	uint8 sock[IPX_SOCK_SIZE];	/* ipx socket */
} ipxSock_t;

#define IPX_TRANSPORT_CONTROL  	0x00
#define IPX_NULL_PACKET_TYPE	0x00
#define IPX_ECHO_PACKET_TYPE	0x02
#define IPX_PEP_PACKET_TYPE		0x04
#define IPX_SPX_PACKET_TYPE		0x05
#define IPX_NOVELL_PACKET_TYPE	0x11
#define IPX_WAN_PACKET_TYPE		0x14

/*
 * Structure defining an ipx header
 */
typedef struct ipxHeader{
	uint16		chksum;		/* checksum FFFF if not done */
	uint16		len;		/* length of data and ipx header */
	uint8		tc;			/* transport control */
	uint8		pt;			/* packet type */
	ipxAddr_t	dest;		/* destination address */
	ipxAddr_t	src;		/* source address */
} ipxHdr_t;

#define ROUTE_TABLE_SIZE 4096

typedef struct routeInfo {
	uint32		net;
	uint16		hops;
	uint16		time;
	uint8		node[IPX_NODE_SIZE];
} routeInfo_t;

#define ROUTE_INFO_SIZE 14

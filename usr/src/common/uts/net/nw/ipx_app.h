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

#ifndef _NET_NW_IPX_APP_H  /* wrapper symbol for kernel use */
#define _NET_NW_IPX_APP_H  /* subject to change without notice */

#ident	"@(#)kern:net/nw/ipx_app.h	1.9"
#ident	"$Id: ipx_app.h,v 1.10.2.1 1994/11/08 20:48:54 vtag Exp $"

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
#else 
#include <sys/types.h>
#include <sys/nwtdr.h>
#endif /* _KERNEL_HEADERS */

#define IPX_DIAGNOSTIC_SOCKET	0x0456

/*
**	The following ioctls apply to the Ipx Socket Multiplexor.  If
**	a version of IPX is running that has no socket Multiplexor, these
**	ioctls will fail.
*/

#define IPX_SET_SOCKET 			( ( 'i' << 8 ) | 2 )
#define IPX_BIND_SOCKET			( ( 'i' << 8 ) | 20 )
#define IPX_UNBIND_SOCKET		( ( 'i' << 8 ) | 21 )
#define IPX_INITIALIZE 			( ( 'i' << 8 ) | 22 )
#define IPX_SET_WATER			( ( 'i' << 8 ) | 23 )

/*
**	IPX_SET_SOCKET - Bind an open file handle to an IPX Socket
**
**        request_socket        0             machine_order
**        return_socket      net_order        machine_order
**
**	This function allows exactly one socket to be bound for the process.
**	To bind another socket, you must first release the socket by using
**	the IPX_UNBIND_SOCKET function.
**
**	When you use this function, ipx fills in the socket number
**	on all packets you send.
*/

/*
**	IPX_BIND_SOCKET - Bind an open file handle to an IPX Socket
**	IPX_UNBIND_SOCKET - Release a bound socket from an open file handle.
**		Can be used multiple times to bind/release different sockets
**		to the same stream.
**	Uses IpxSetSocket_t structure
**
**        request_socket     machine_order   (BIND/UNBIND)
**        return_socket      machine_order   (BIND)
**
**	This function allows multiple sockets to be bound for a process.
**	Use this function once for each socket to be bound.
**	When you use this function, you must fill in the socket number
**	on all packets you send.  If the non-zero socket number you fill in is
**	invalid, the packet is discarded.  If you send zero as a socket number,
**	an M_ERROR is sent to the stream head with a status of EINVAL.
*/

typedef struct {
	uint16 socketNum;		/* Socket number */
} IpxSetSocket_t;

/*
**	IPX_SET_WATER - Set stream head read High and Low Water mark.
**		Values of 0xFFFFFFFF (UINT_MAX) will set the water mark
**		to the computed maximum set during IPX_INITIALIZE.
**	IPX_INITIALIZE - Indicate to IPX that initialization is complete
**		and set max allowable hi/lo water values for non root users.
**		Uses IpxSetWater_t ioctl structure for IPX_INITIALIZE ioctl.
*/
typedef struct {
	uint32 hiWater;
	uint32 loWater;
} IpxSetWater_t;

/*
**	IPX_SET_SPX - Following link of IPX under SPX, this ioctl is
**		sent to identify stream as SPX stream.  Requires no data.
*/

/*
**	IPX_STATS - Get IPX statistics
**		This structure follows the IpxLanStats_t structure.
**		The buffer must be big enough to hold both.  If The IPX
**		Socket Multiplexor is not present, all values will be zero.
*/
typedef struct {
	uint8	IpxMajorVersion;		/* Major Version */ 
	uint8	IpxMinorVersion;		/* Minor Version */ 
	char	IpxRevision[2];			/* Revision */ 
	uint32	IpxOutData;				/* Number of non TLI output data packets */
	uint32	IpxOutBadSize;			/* Non TLI data pkts w/size < IPX hdr len */
	uint32	IpxOutToSwitch;			/* Number packet sent to LAN router */
	uint32	IpxBoundSockets;		/* Number sockets currently bound */
	uint32	IpxBind;				/* Number of Non TLI bind requests */
	uint32	IpxTLIOutData;			/* TLI Total number of output data pkts */
	uint32	IpxTLIOutBadState;		/* TLI data, socket not bound, drop */
	uint32	IpxTLIOutBadSize;		/* TLI Data Request is wrong size, drop */
	uint32	IpxTLIOutBadAddr;		/* TLI Req'st IPX addr is bad size, drop */
	uint32	IpxTLIOutBadOpt;		/* TLI Request has bad TLI opt size, drop */
	uint32	IpxTLIOutHdrAlloc;		/* TLI Req'st IPX hdr alloc failure, drop */
	uint32	IpxTLIOutToSwitch;		/* TLI Packets sent to the LAN router */
	uint32	IpxTLIBind;				/* TLI Bind Requests */
	uint32	IpxTLIOptMgt;			/* TLI Number of opt_mgmt requests */
	uint32	IpxTLIUnknown;			/* TLI Number of unknown requests, drop'd */
	uint32	IpxInSwitch;			/* Total Packets Propagated */
	uint32	IpxSwitchInvalSocket;	/* BIND_SOCK usr, invalid src sock, drop */
	uint32	IpxSwitchSumFail;		/* number of pkts, can't gen cksum, drop */
	uint32	IpxSwitchSum;			/* number of packets checksum generated */
	uint32	IpxSwitchAllocFail;		/* num pkts, can't get space to pad, drop */
	uint32	IpxSwitchEven;			/* number of packets padded to even bytes */
	uint32	IpxSwitchEvenAlloc;		/* number of pkts new buffer req'd to pad */
	uint32	IpxSwitchToLan;			/* Switch lan - sent to lan driver */
	uint32	IpxInData;				/* Total data received by socket mux */
	uint32	IpxInBadSize;			/* Not Used */
	uint32	IpxDataToSocket;		/* Packets routed to a socket */
	uint32	IpxTrimPacket;			/* Not USED, moved to lipmx_app.h stats */
	uint32	IpxSumFail;				/* Packet checksum invalid, dropped */
	uint32	IpxRouted;				/* Packet routed to a non TLI socket */
	uint32	IpxRoutedTLI;			/* Packet routed to TLI socket */
	uint32	IpxRoutedTLIAlloc;		/* Packet, alloc of TLI hdr fail, drop */
	uint32	IpxBusySocket;			/* Destination socket stream full, drop */
	uint32	IpxSocketNotBound;		/* Destination socket not bound, drop */
	uint32	IpxIoctlBindSocket;		/* Ioctl SET/BIND_SOCKET */
	uint32	IpxIoctlUnbindSocket;	/* Ioctl UNBIND_SOCKET */
	uint32	IpxIoctlStats;			/* Ioctl STATS */
	uint32	IpxIoctlUnknown;		/* Ioctl Sent to LAN router */
	uint32	IpxIoctlSetWater;		/* Ioctl SET_WATER */
} IpxSocketStats_t;

#define IPX_MAX_PACKET_DATA 546

#define IPX_HDR_SIZE		30
#define IPX_ADDR_SIZE		12
#define IPX_CHKSUM			0xFFFF
#define IPX_CHKSUM_TRIGGER	GETINT16(0xFEEB)
#define IPX_NET_SIZE		4
#define IPX_NODE_SIZE		6
#define IPX_SOCK_SIZE		2

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

typedef struct ipxCAddr {
	uint8 addr[IPX_ADDR_SIZE];	/* ipx address */
} ipxCAddr_t;

typedef struct ipxCHdr {
	uint8 hdr[IPX_HDR_SIZE];	/* ipx header */
} ipxCHdr_t;

#define IPX_TRANSPORT_CONTROL  	0x00

#define IPX_NULL_PACKET_TYPE	0x00
#define IPX_RIP_PACKET_TYPE		0x01
#define IPX_ECHO_PACKET_TYPE	0x02
#define IPX_PEP_PACKET_TYPE		0x04
#define IPX_SPX_PACKET_TYPE		0x05
#define IPX_NOVELL_PACKET_TYPE	0x11

#define IPX_PROPAGATION_TYPE	0x14	/* Broadcast propogation (NetBIOS) */
#define IPX_PROPAGATION_MAX_NETS 8
#define IPX_PROPAGATION_LENGTH	(IPX_NET_SIZE * IPX_PROPAGATION_MAX_NETS)

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

/*
** Faster address comparison than a bcmp/memcmp or equiv.
**
**	Compare in order most likely to find differences
*/

#define IPXCMPNET(a,b) ( \
	((char *)(a))[3] == ((char *)(b))[3] &&\
	((char *)(a))[2] == ((char *)(b))[2] &&\
	((char *)(a))[1] == ((char *)(b))[1] &&\
	((char *)(a))[0] == ((char *)(b))[0] \
)

#define IPXCMPNODE(a,b) ( \
	((char *)(a))[5] == ((char *)(b))[5] &&\
	((char *)(a))[4] == ((char *)(b))[4] &&\
	((char *)(a))[3] == ((char *)(b))[3] &&\
	((char *)(a))[2] == ((char *)(b))[2] &&\
	((char *)(a))[1] == ((char *)(b))[1] &&\
	((char *)(a))[0] == ((char *)(b))[0] \
)

#define IPXCMPSOCK(a,b) ( \
	((char *)(a))[1] == ((char *)(b))[1] &&\
	((char *)(a))[0] == ((char *)(b))[0] \
)

/*
**	Comparing server addresses: net addresses are unique
**								node addresses are the same (000001)
**								socket addresses likely are 04xx
**	Comparing client addresses: net addresses are the same on a net
**								node addresses are unique (NIC address)
**								socket addresses are similar (but not the same)
**	Compare in an order to determine uniqueness quickly for either case.
**	We favor comparing server addresses in the test, but are fair for client
**	addresses as well
*/
#define IPXCMPADDR(a,b) ( \
	((char *)(a))[3]  == ((char *)(b))[3]  &&	/* net[3] */	\
	((char *)(a))[11] == ((char *)(b))[11] &&	/* socket[1] */	\
	((char *)(a))[9]  == ((char *)(b))[9]  &&	/* node[5] */	\
	((char *)(a))[8]  == ((char *)(b))[8]  &&	/* node[4] */	\
	((char *)(a))[2]  == ((char *)(b))[2]  &&	/* net[2] */	\
	((char *)(a))[10] == ((char *)(b))[10] &&	/* socket[0] */	\
	((char *)(a))[1]  == ((char *)(b))[1]  &&	/* net[1] */	\
	((char *)(a))[7]  == ((char *)(b))[7]  &&	/* node[3] */	\
	((char *)(a))[6]  == ((char *)(b))[6]  &&	/* node[2] */	\
	((char *)(a))[0]  == ((char *)(b))[0]  &&	/* net[0] */	\
	((char *)(a))[5]  == ((char *)(b))[5]  &&	/* node[1] */	\
	((char *)(a))[4]  == ((char *)(b))[4] 		/* node[0] */	\
)
/* 
** Compare using the technique for IPXCMPADDR just the net and node part
** of the address (used for comparing client's address where the socket
** is different for regular NCPs than for Packet Burst NCPs)
*/

#define IPXCMPNETNODE(a,b) ( \
	((char *)(a))[3]  == ((char *)(b))[3]  &&	/* net[3] */	\
	((char *)(a))[9]  == ((char *)(b))[9]  &&	/* node[5] */	\
	((char *)(a))[8]  == ((char *)(b))[8]  &&	/* node[4] */	\
	((char *)(a))[2]  == ((char *)(b))[2]  &&	/* net[2] */	\
	((char *)(a))[1]  == ((char *)(b))[1]  &&	/* net[1] */	\
	((char *)(a))[7]  == ((char *)(b))[7]  &&	/* node[3] */	\
	((char *)(a))[6]  == ((char *)(b))[6]  &&	/* node[2] */	\
	((char *)(a))[0]  == ((char *)(b))[0]  &&	/* net[0] */	\
	((char *)(a))[5]  == ((char *)(b))[5]  &&	/* node[1] */	\
	((char *)(a))[4]  == ((char *)(b))[4] 		/* node[0] */	\
)

/*
 * Faster address copy than a bcopy/memcpy or equiv.
 */

#define IPXCOPYNET(a,b) ( \
	(*((ipxNet_t *)(b))) = (*((ipxNet_t *)(a))) \
)

#define IPXCOPYNODE(a,b) ( \
	(*((ipxNode_t *)(b))) = (*((ipxNode_t *)(a))) \
)

#define IPXCOPYSOCK(a,b) ( \
	(*((ipxSock_t *)(b))) = (*((ipxSock_t *)(a))) \
)

#define IPXCOPYADDR(a,b) ( \
	(*((ipxCAddr_t *)(b))) = (*((ipxCAddr_t *)(a))) \
)

#define IPXCOPYHDR(a,b) ( \
	(*((ipxCHdr_t *)(b))) = (*((ipxCHdr_t *)(a))) \
)

#ifdef _KERNEL_HEADERS
#include <net/nw/lipmx_app.h>
#else 
#include <sys/lipmx_app.h>
#endif /* _KERNEL_HEADERS */

#endif /* _NET_NW_IPX_APP_H */

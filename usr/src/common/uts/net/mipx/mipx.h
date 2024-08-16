/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/mipx/mipx.h	1.7"
/*
 *        Copyright Univel Inc. 1991
 *        (C) Unpublished Copyright of Univel, Inc. All Rights Reserved.
 *
 *        No part of this file may be duplicated, revised, translated, localized
 *        or modified in any manner or compiled, linked or uploaded or
 *        downloaded to or from any computer system without the prior written
 *        consent of Univel, Inc.
 *
 *
 *   	  Merge Ipx Driver 
 *        Author: Srikant Mukherjee
 *
 *
 */

#ifdef	_KERNEL_HEADERS
#include	<util/types.h>
#include	<io/stream.h>
#include	<util/param.h>
#include	<io/stropts.h>
#include	<util/sysmacros.h>
#include	<svc/errno.h>
#include	<proc/cred.h>
#ifdef	MP
#include	<util/ksynch.h>
#endif
#include	<util/cmn_err.h>
#include	<io/ddi.h>
#include	<util/mod/moddefs.h>
#else
#include	<sys/types.h>
#include	<sys/stream.h>
#include	<sys/param.h>
#include	<sys/stropts.h>
#include	<sys/sysmacros.h>
#include	<sys/errno.h>
#include	<sys/cred.h>
#ifdef	MP
#include	<sys/ksynch.h>
#endif
#include	<sys/cmn_err.h>
#include	<sys/ddi.h>
#include	<sys/moddefs.h>
#endif

#define IPX_MIN_CONNECTED_LANS 1
#define IPX_MAX_CONNECTED_LANS 1

#define IPX_MAX_PACKET_DATA 546

#define IPX_HDR_SIZE		30
#define IPX_ADDR_SIZE		12
#define IPX_CHKSUM			0xFFFF
#define IPX_NET_SIZE		4
#define IPX_NODE_SIZE		6
#define IPX_SOCK_SIZE		2
#define MIPXID 			500 /* streams logger id */
#define IPX_OPENSOCK 		0
#define IPX_CLOSESOCK 		2
#define IPX_DIE 		3
#define MIPX_IMREADY		4
#define IPX_GETMORESTREAMS 	1
#define MIPX_LO_WATER	 	40000
#define MIPX_HI_WATER	 	50000
#define MIPX_UNUSED	 	0
#define MIPX_USED	 	1
#define MIPX_WDOG_PENDING	2
#define MIPX_CONN_ESTABLISHED	4
#define MIPX_CNTL_STR	 	2
#define MIPX_SET_SOCKET	 	0xff
#define MIPX_SET_NET	 	0xfe
#define MIPX_REL_DOS	 	0xfd
#define MIPX_SEND_DISCONNECT 	0xfc
#define SUCCESS		 	 0
#define FAIL		 	-1
#define MIPX_INIT		 0
#define FROM_MPIP		 1

/*
 * Structure defining an ipx address
 */
typedef struct ipxAddress {
	unsigned char  	net[IPX_NET_SIZE];		/* ipx network address */
	unsigned char 	node[IPX_NODE_SIZE];	/* ipx node address */
	unsigned char 	sock[IPX_SOCK_SIZE];	/* ipx socket */
} ipxAddr_t;

typedef struct ipxNet {
	unsigned char net[IPX_NET_SIZE];	/* ipx network address */
} ipxNet_t;

typedef struct ipxNode {
	unsigned char node[IPX_NODE_SIZE];	/* ipx node address */
} ipxNode_t;

typedef struct ipxSock {
	unsigned char sock[IPX_SOCK_SIZE];	/* ipx socket */
} ipxSock_t;

#define IPX_TRANSPORT_CONTROL  	0x01
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
	unsigned short		chksum;		/* checksum FFFF if not done */
	unsigned short		len;		/* length of data and ipx header */
	unsigned char		tc;			/* transport control */
	unsigned char		pt;			/* packet type */
	ipxAddr_t	dest;		/* destination address */
	ipxAddr_t	src;		/* source address */
} ipxHdr_t;

#define ROUTE_TABLE_SIZE 512

#define MTYPE(x) ((x)->b_datap->db_type)
#define DATA_SIZE(mp)	((mp)->b_wptr - (mp)->b_rptr)
/*
 * Faster address comparation than a bcmp/memcmp or equiv.
 */

#define IPXCMPNET(a,b) ( \
	((char *)(a))[0] == ((char *)(b))[0] &&\
	((char *)(a))[1] == ((char *)(b))[1] &&\
	((char *)(a))[2] == ((char *)(b))[2] &&\
	((char *)(a))[3] == ((char *)(b))[3] \
)

#define IPXCMPNODE(a,b) ( \
	((char *)(a))[0] == ((char *)(b))[0] &&\
	((char *)(a))[1] == ((char *)(b))[1] &&\
	((char *)(a))[2] == ((char *)(b))[2] &&\
	((char *)(a))[3] == ((char *)(b))[3] &&\
	((char *)(a))[4] == ((char *)(b))[4] &&\
	((char *)(a))[5] == ((char *)(b))[5] \
)

#define IPXCMPSOCK(a,b) ( \
	((char *)(a))[0] == ((char *)(b))[0] &&\
	((char *)(a))[1] == ((char *)(b))[1] \
)

#define IPXCOPYSOCK(a,b) ( bcopy ((char *)a, (char *)b, 2))


#define XCHNG(x)		( ((unsigned short)(x & 0x00FF) << 8) | \
				((unsigned short)(x & 0xFF00) >> 8) \
				)
/*
 * Copy with possible alignment problems.
 */
#define IPX_MAX_SOCKETS 100 
#define MIPX_DATA_SIZE 360 
#define IPX_NET_HASH_TABLE_SIZE 16
#define IPX_MAX_CONNECTED_LANS 1

#define MIPX_INCOMING_WDOG(hptr, mp) ((hptr->len == 0x2000 ) && (hptr->pt == 0 ) && (*(mp->b_rptr + 31) == '?'))

#define MIPX_CONNECTION_RSP(nptr) ((nptr->len == 0x2600) && (nptr->reqType == 0x3333) && (nptr->seqNum == 0) && (nptr->reqCode == 0))

#define MIPX_DISCONNECT_RSP(nptr, mdp) ((nptr->len == 0x2600) && (nptr->reqType == 0x3333) && (nptr->seqNum == mdp->ncpseqnum) && (nptr->reqCode == 0))

#define MIPX_OUTGOING_WDOG(hptr, mp) ((hptr->len == 0x2000 ) && (hptr->pt == 0 ) && (*(mp->b_rptr + 31) == 'Y'))

#define IPX_NET_NULL(a) ( \
	((char *)(a))[0] == 0 &&\
	((char *)(a))[1] == 0 &&\
	((char *)(a))[2] == 0 &&\
	((char *)(a))[3] == 0 \
)

#define CONN_RSP(x) ( \
	((unsigned short) *(x + 30) == 0x3333) && \
	((unsigned char) *(x + 32) == 0x0) && \
	((unsigned char) *(x + 34) == 0x0) && \
	((unsigned char) *(x + 36) == 0x0) \
)
/* 
 * Private data structure for each open stream to the ipx driver
 */
struct	setsock {
	int	l_index;
	unsigned char sock[IPX_SOCK_SIZE];
};

typedef struct ipxconninfo {
	ipxAddr_t   ServerAddr;
	unsigned char	connNum;
} ipxConnInfo_t;

typedef struct mipxData {
	queue_t		*ipx_qptr;
	queue_t		*mpip_qptr;
	ipxSock_t	dsock;		/* ipx socket to which packets from 
					   this queue are to be routted */
	ipxNet_t	dnet;
	int		l_index;
	int		timeoutID;
	ipxConnInfo_t	*cinfo;
	unsigned char	state;
	unsigned char	ncpseqnum;
	short		count; /* the number of ipx links  that this 
				dos-merge process has left to use. This
				field is useful only if this particular
				element is at the head of the queue */
	struct mipxData	*prev;
	struct mipxData	*next;
} mipxData_t;

typedef struct ipxNcpReq {
	unsigned short		chksum;	/* checksum FFFF if not done */
	unsigned short		len;	/* length of data and ipx header */
	unsigned char		tc;	/* transport control */
	unsigned char		pt;	/* packet type */
	ipxAddr_t	dest;		/* destination address */
	ipxAddr_t	src;		/* source address */
	unsigned short  reqType;
	unsigned char  seqNum;
	unsigned char  connNum;
	unsigned char	taskNum;
	unsigned char   reserved;
	unsigned char   reqCode;
	unsigned char   statFlags;
} ipxNcpReq_t;

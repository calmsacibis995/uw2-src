/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ripx/rip_adt.h	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_RIPX_RIP_ADT_H  /* wrapper symbol for kernel use */
#define _NET_NW_RIPX_RIP_ADT_H  /* subject to change without notice */

#ident	"$Id: rip_adt.h,v 1.3 1994/02/18 15:23:09 vtag Exp $"

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
#include <net/nw/ipx_app.h>
#include <net/nw/ripx_app.h>
#else
#include "sys/ipx_app.h"
#include "sys/ripx_app.h"
#endif /* _KERNEL_HEADERS */

#define MAX_MAX_HOPS	0x10

typedef struct netRouteEntry {
	struct netRouteEntry	*nextRouteLink;
	void	*rrLanKey;
	uint16	timer;
	uint16	routeTime;
	uint8	routeHops;
	uint8	routeStatus;
	uint8	forwardingRouter[6];
} netRouteEntry_t;

typedef struct netListEntry {
	uint32				netIDNumber;
	uint16				timeToNet;
	uint8				hopsToNet;
	uint8				netStatus;
	uint8				usedRouterModCount;
	int8				entryChanged;
	int8				routerLostNetFlag;
	int8				mustAdvertise;
	uint32				netHashIndex;
	struct netListEntry *hash_next;
	struct netListEntry *hash_prev;
	struct netListEntry *listNext;
	netRouteEntry_t		*routeListLink;
} netListEntry_t;

typedef struct routePtrPtr {
	netRouteEntry_t	*routePtr;
} routePtrPtr_t;

typedef struct routeEntry {
	uint32		targetNet;
	uint16		targetHops;
	uint16		targetTime;
} routeEntry_t;

typedef struct routePacket {
	uint16		chksum;		/* checksum FFFF if not done */
	uint16		len;		/* length of data and ipx header */
	uint8		tc;			/* transport control */
	uint8		pt;			/* packet type */
	ipxAddr_t	dest;		/* destination address */
	ipxAddr_t	src;		/* source address */
	uint16		operation;
	routeEntry_t	routeTable[1];
} routePacket_t;

#endif /* _NET_NW_RIPX_RIP_ADT_H */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ripx/rip.h	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_RIPX_RIP_H  /* wrapper symbol for kernel use */
#define _NET_NW_RIPX_RIP_H  /* subject to change without notice */

#ident	"$Id: rip.h,v 1.7 1994/09/14 16:45:30 meb Exp $"

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
#include <net/nw/nwcommon.h>
#include <net/nw/ripx/rip_adt.h>
#else
#include "nwcommon.h"
#include "rip_adt.h"
#endif /* _KERNEL_HEADERS */

#ifdef NTR_TRACING
#define	NTR_ModMask		NTRM_ripx
#endif

/* A RIT hash bucket will be pointer to a list of net entries,
 * and a lock protecting the list.
 */
typedef struct {
	netListEntry_t	*list;
	lock_t		*lock;
} NetHash_t;

/*
 * The Routing Information Table (RIT)
 */
typedef struct {
	uint32			size; /* use mask,size as uint16, keep as uint32 */
	NetHash_t		*table;
	uint32			mask;	/* for atomic word operations */
	hashStats_t		stats;
} RtInfoTable_t;

/*
**  Statistics
*/
extern RouterInfo_t ripStats;

/* router defines */
#define ROUTE_ENTRY_SIZE	8
#define ROUTE_PACKET_SIZE	32
#define MIN_ROUTE_PACKET	40

#define NET_REFER		-1
#define FULL_INFO		1

extern void	RipAdjustTimerInterval(uint16);
extern void	RipResetRouter(void);
extern void	RipDownRouter(void);
extern void	RipInit(void);
extern void	RipUntimeout(void);
extern void	RipSendRouterInfo(void *, uint8 *, uint8 *, uint8 *, int);
extern void	RipHashStats(hashStats_t *);
extern int	RipHashBucketCounts(int *, uint16);
extern int	RIPmapNetToIpxLanKey(uint32 targetNet, void **ipxLanKeyPtr);
extern mblk_t	*RIPgetRouteInfo(ipxHdr_t *, void **lanIndexPtr);
extern void	RIPcheckSapSource(checkSapSource_t *);
extern int	RIPgetNetList(netInfo_t *);
extern int	RIPgetRouterTable(queue_t *q);
extern int	RIPInitialize(Initialize_t *initTable);
extern void	RIPstart(queue_t *ripQ);
extern void	RIPfinish(void);
extern void	RipDeconfigureLocalLan(void *rrLanKey, uint32 ipxNetAddr);
extern int	RIPgetNetInfo(netInfo_t *netInfo);

#endif /* _NET_NW_RIPX_RIP_H */

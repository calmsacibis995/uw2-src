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

#ifndef _NET_NW_IPX_LIPMX_LIPMX_H  /* wrapper symbol for kernel use */
#define _NET_NW_IPX_LIPMX_LIPMX_H  /* subject to change without notice */

#ident	"@(#)kern:net/nw/ipx/lipmx/lipmx.h	1.5"
#ident	"$Id: lipmx.h,v 1.6.2.1 1994/11/08 20:49:30 vtag Exp $"

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

/*
 * lipmx   - Novell Streams LIPMX Driver
 */

#ifdef _KERNEL_HEADERS
#include <net/nw/nwcommon.h>
#include <net/dlpi.h>
#include <net/nw/ipx_app.h>
#include <net/nw/ripx_app.h>
#include <net/nw/ipxspx_diag.h>
#include <net/nw/rrouter.h>
#include <net/nw/ipx_lipmx.h>
#include <net/nw/spxipx.h>
#else
#include "nwcommon.h"
#include <sys/dlpi.h>
#include "sys/ipx_app.h"
#include "sys/ripx_app.h"
#include "sys/ipxspx_diag.h"
#include "rrouter.h"
#include "ipx_lipmx.h"
#include "spxipx.h"
#endif /* _KERNEL_HEADERS */

#ifdef NTR_TRACING
#define NTR_ModMask     NTRM_lipmx
#endif

/*
**  Router Methods LIPMX supplies to RIPX
*/
extern void		LIPMXsendData(mblk_t *, void **, uint8);
extern uint32	LIPMXmapSapLanToNetwork(uint32);
extern void    *LIPMXuseMethodWithLanKey(void *,void *,void *(void *, void *));

/*
**  Lipmx shared functions
*/
extern void		LipmxClearUnderIpx(void);
extern void		LipmxClearUQ(void);
extern void		LipmxDlUnitdataInd(queue_t *, mblk_t *);
extern void		LipmxDlUnitdataInd(queue_t *, mblk_t *);
extern void		LipmxDupBroadcastPacket(queue_t *, mblk_t *);
extern uint32	LipmxGetConfiguredLans(void);
extern int  	LipmxSetConfiguredLans(uint32);
extern uint16	LipmxGetMaxHops();
extern void		LipmxSetMaxHops(uint16);
extern int 		LipmxGetLanInfo(lanInfo_t *);
extern int  	LipmxLinkLan(struct linkblk *);
extern int  	LipmxSetLanInfo(lanInfo_t *);
extern int  	LipmxSetUQ(queue_t *);
extern void 	LipmxStreamError(queue_t *, mblk_t *);
extern int  	LipmxTestUQ(void);
extern int  	LipmxUnlinkLan(int);
extern void		RegisterLansWithRRouter(void);
extern void		DivestRRouterLanKeys(void);
extern mblk_t  *LipmxTrimPacket( mblk_t *);

#define IPX_SAP_SIZE sizeof(uint16)

/* router defines
*/
#define ROUTE_ENTRIES_PER_PACKET 50
#define ROUTE_REQUEST 1
#define ROUTE_RESPONSE 2
#define MIN_ROUTE_PACKET 40
#define ROUTE_PACKET_SIZE 32
#define ROUTE_ENTRY_SIZE 8
#define ACCEPT_SERVER_OPERATION 2
#define IPX_ROUTER_DOWN 127
#define CHECK -1
#define NOCHECK 0
#define ALL -1
#define CHANGED (uint8)0x01
#define SENT 0x02

/* status bits */
#define CHANGED_ONLY_BIT    0x80
#endif /* _NET_NW_IPX_LIPMX_LIPMX_H */

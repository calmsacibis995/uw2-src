/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ripx/rrrip.h	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_RIPX_RRRIP_H  /* wrapper symbol for kernel use */
#define _NET_NW_RIPX_RRRIP_H  /* subject to change without notice */

#ident	"$Id: rrrip.h,v 1.6 1994/08/11 16:34:20 meb Exp $"

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
/* rrrip.h
 *
 * RIP's contribution to the Replaceable Router interface.
 */

#ifdef _KERNEL_HEADERS
#include <net/nw/rrouter.h>
#include <net/nw/ipx_app.h>
#else
#include "rrouter.h"
#include "sys/ipx_app.h"
#endif /* _KERNEL_HEADERS */

/* RIPSendRouterInfoSWITCH cases */
#define ADVERTISE	1
#define POST_LOST	2
#define ACCEPT_INFO	3
#define AGE_SLOW	4
#define AGE_NORMAL	5
#define DOWN_ROUTER	6

extern uint16	RipMaxLanPkt(void *lanKey);
extern uint16	RipLanTime(void *lanKey);
extern void 	RIPClearChanged(void);

/*
**	Replaceable Router Ioctl Functions
*/
void	RIPISetSapQ(queue_t *, mblk_t *);
extern void	RIPInitRouter(queue_t *, mblk_t *);
extern void	RIPStartRouter(queue_t *, mblk_t *);
extern void	RIPIResetRouter(queue_t *, mblk_t *);
extern void	RIPIDownRouter(queue_t *, mblk_t *);
extern void	RIPIGetNetInfo(queue_t *, mblk_t *);
extern void	RIPIGetRouterTable(queue_t *, mblk_t *);
extern void	RIPIInfo(queue_t *, mblk_t *);
extern void	RIPICheckSapSource(queue_t *, mblk_t *);

/*
**	Replaceable Router Interface Functions
*/
extern void	 RIPdigestRouterPacket(void *, mblk_t *);
extern int	 RIPChkNetConflict(void *, uint8 *, uint8 *);
extern void	 RipRegisterRouter(void);
extern void	 RipDeregisterRouter(void);

extern int	 RIPAddRouterInfo(uint32, uint8 *, uint16, uint16, void *, uint8);
extern uint32	RIPLanData(void *, void **, uint8 *, uint16 *);
extern uint8 RIPNetStatus(void *);
extern void	*RIPSendRouterInfoSWITCH(void *, void *);
extern void	 RIPSendUpdates(int, long);
extern mblk_t	*RIPmakeDlHdr(ipxHdr_t *hdr, void **ipxLanKeyPtr,
								netRouteEntry_t *route);
extern void	RipDispatchPkt(void *lanKey, mblk_t *mp, uint8 flags);
extern int	RIPmapRRKeyToIpxKey(void *rrLanKey, void **ipxLanKeyPtr);
extern void RipLanDeathScream(void *routerKey);

#endif /* _NET_NW_RIPX_RRRIP_H */

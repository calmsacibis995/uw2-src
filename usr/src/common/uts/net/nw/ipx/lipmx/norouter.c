/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/lipmx/norouter.c	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: norouter.c,v 1.3 1994/02/18 15:21:20 vtag Exp $"
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
#include <net/nw/ipx/lipmx/norouter.h>
#else
#include "norouter.h"
#endif

FSTATIC void	*NoRouterGrantLanKey(rrLanData_t *);
FSTATIC void	 NoRouterUpdateLanKey(void *, rrLanData_t *);
FSTATIC void	*NoRouterInvalidateLanKey(void *);
FSTATIC void	 NoRouterDigestRouterPacket(void *, mblk_t *);
FSTATIC int		 NoRouterCheckSapPacket(void *, mblk_t *);
FSTATIC int		 NoRouterMapNetToIpxLanKey(uint32, void **);
FSTATIC mblk_t	*NoRouterGetRouteInfo(ipxHdr_t *, void **);

/*
 * void LipmxInitNoRouter(void)
 *	Initialize NoRouter by calling RegisterRRouter with function pointers
 *	to NoRouter functions.
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
void
LipmxInitNoRouter(void)
{	static RROUTERMethods_t	NoRouter = {
		NoRouterGrantLanKey,
		NoRouterUpdateLanKey,
		NoRouterInvalidateLanKey,
		NoRouterDigestRouterPacket,
#ifdef OLD_SAP_CHECKING
		NoRouterCheckSapPacket,
#endif
		NoRouterMapNetToIpxLanKey,
		NoRouterGetRouteInfo
	};
	NTR_ENTER(0, 0,0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter LipmxInitNoRouter"));
	RRIPX.RegisterRRouter(&NoRouter);
	NTR_VLEAVE();
	return;
}

/*
 * void * NoRouterGrantLanKey(rrLanData_t *lanData)
 *	Null function for the NoRouter
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
/*ARGSUSED*/
void *
NoRouterGrantLanKey(rrLanData_t *lanData)
{	NTR_ENTER(1, lanData, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter NoRouterGrantLanKey"));
	if(lanData->prepend)
		freemsg(lanData->prepend);
	return((void *)NTR_LEAVE(0));
}

/*
 * void NoRouterUpdateLanKey(void *lanKey, rrLanData_t *lanData)
 *	Null function for the NoRouter
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
/*ARGSUSED*/
void
NoRouterUpdateLanKey(void *lanKey, rrLanData_t *lanData)
{	NTR_ENTER(2, lanKey, lanData, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter NoRouterUpdateLanKey"));
	NTR_VLEAVE();
	return;
}

/*
 * void * NoRouterInvalidateLanKey(void *lanKey)
 *	Null function for the NoRouter
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
/*ARGSUSED*/
void *
NoRouterInvalidateLanKey(void *lanKey)
{	NTR_ENTER(1, lanKey, 0,0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter NoRouterInvalidateLanKey"));
	return((void *)NTR_LEAVE(0));
}

/*
 * void NoRouterDigestRouterPacket(void *lanKey, mblk_t *mp)
 *	Null function for the NoRouter
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
/*ARGSUSED*/
void
NoRouterDigestRouterPacket(void *lanKey, mblk_t *mp)
{	NTR_ENTER(2, lanKey, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter NoRouterDigestRouterPacket"));
	/* DON'T freemsg(mp)!, lipmx will want to route pkt! */
	NTR_VLEAVE();
	return;
}

#ifdef OLD_SAP_CHECKING
/*
 * int NoRouterCheckSapPacket(void *lanKey, mblk_t *mp)
 *	Null function for the NoRouter
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
/*ARGSUSED*/
int
NoRouterCheckSapPacket(void *lanKey, mblk_t *mp)
{	NTR_ENTER(2, lanKey, mp, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter NoRouterCheckSapPacket"));
	return(NTR_LEAVE(-1));
}
#endif

/*
 * int NoRouterMapNetToIpxLanKey(uint32 net, void **ipxLanKey)
 *	Null function for the NoRouter
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
/*ARGSUSED*/
int
NoRouterMapNetToIpxLanKey(uint32 net, void **ipxLanKey)
{	NTR_ENTER(2, net, ipxLanKey, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter NoRouterGetLanToNet"));
	if (ipxLanKey != NULL)
		*((uint32 *)ipxLanKey) = (uint32)-1;
	return(NTR_LEAVE(0));
}

/*
 * mblk_t * NoRouterGetRouteInfo(ipxHdr_t *hdr, void **ipxLanKey)
 *	Null function for the NoRouter
 *
 * Calling/Exit State:
 *	No locks on entry or exit.
 *	
 */
/*ARGSUSED*/
mblk_t *
NoRouterGetRouteInfo(ipxHdr_t *hdr, void **ipxLanKey)
{	NTR_ENTER(2, hdr, ipxLanKey, 0,0,0);
	NWSLOG((LIPMXID,0,PNW_ENTER_ROUTINE,SL_TRACE,
			"Enter NoRouterGetRouteInfo"));
	if (ipxLanKey != NULL)
		*((uint32 *)ipxLanKey) = (uint32)-1;
	return((mblk_t *)NTR_LEAVE(NULL));
}

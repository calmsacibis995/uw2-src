/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx_lipmx.h	1.7"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_IPX_LIPMX_H  /* wrapper symbol for kernel use */
#define _NET_NW_IPX_LIPMX_H  /* subject to change without notice */

#ident	"$Id: ipx_lipmx.h,v 1.5 1994/05/16 13:53:49 vtag Exp $"

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
 *
 */

/*
**	Functions in lipmx
*/
extern int	lipmxuwput(queue_t *q, mblk_t *mp);	/* Send data to lan */
extern int	lipmxlrput(queue_t *q, mblk_t *mp);	/* Get data from lan */
extern int	lipmxlrsrv(queue_t *q);				/* Lower read service */
extern int	lipmxlwsrv(queue_t *q);				/* Lower write service */
extern int	LipmxGetMaxSDU(uint32 net);			/* Get max SDU of a net */
extern void	LipmxSetUnderIpx(void);				/* Indicate running under ipx */
extern void	LipmxMinMaxSDU(void);				/* Get smallest of maxSDs */
extern void lipmxinit(void);					/* Driver initialization */
extern void lipmxCleanUp(void);					/* Driver de-initialization */
extern void LipmxRouteRIP(int);					/* Route Rip Packets to ipx */
extern int  LipmxGetMinMaxSDU( void);			/* Get Lan Min/Max SDU */

/*
**	Constants to passed in LipmxRouteRIP
*/
#define ENABLE_RIP	1
#define DISABLE_RIP 0

/*
**	Functions in both ipx and lipmx
*/
extern int	IsPriv(queue_t *q);					/* Is q root */
extern int	IsControl(queue_t *q);				/* Is q control device */

/*
**	Functions in ipx
*/
extern void	IpxRouteDataToSocket( mblk_t *mp);	/* ipx read (upstream) data */
extern void IpxSetInternalNetNode(uint32 *, uint8 *);/* Set internal net/node */

#define IPX_R_HI_WATER 15000
#define IPX_R_LO_WATER 12000

#define IPX_W_HI_WATER 2500
#define IPX_W_LO_WATER 2000

/*
**  Define Lock Hierarchy
*/
#define DEVICE_LOCK		30
#define QUEUE_LOCK		31
#define HASH_LOCK		32

#define IPXRSAP_LOCK	31
#define LAN_LOCK		32		/* lipmx Lan_t lock */

#endif /* _NET_NW_IPX_LIPMX_H */

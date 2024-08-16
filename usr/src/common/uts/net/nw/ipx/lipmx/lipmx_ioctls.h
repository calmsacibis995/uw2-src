/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/lipmx/lipmx_ioctls.h	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_IPX_LIPMX_LIPMX_IOCTLS_H  /* wrapper symbol for kernel use */
#define _NET_NW_IPX_LIPMX_LIPMX_IOCTLS_H  /* subject to change without notice */

#ident	"$Id: lipmx_ioctls.h,v 1.3 1994/02/18 15:21:35 vtag Exp $"

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

#ifdef _KERNEL_HEADERS
#include <net/nw/ipx/lipmx/lipmx.h>
#else
#include "lipmx.h"
#endif /* _KERNEL_HEADERS */

extern void	LipmxIocGetConfiguredLans(queue_t *, mblk_t *);
extern void	LipmxIocGetLanInfo(queue_t *, mblk_t *);
extern void	LipmxIocGetNetAddr(queue_t *, mblk_t *);
extern void	LipmxIocGetNodeAddr(queue_t *, mblk_t *);
extern void	LipmxIocLink(queue_t *, mblk_t *);
extern void	LipmxIocSetConfiguredLans(queue_t *, mblk_t *);
extern void	LipmxIocSetLanInfo(queue_t *, mblk_t *);
extern void	LipmxIocSetSapQ(queue_t *, mblk_t *);
extern void	LipmxIocStats(queue_t *, mblk_t *);
extern void	LipmxIocUnlink(queue_t *, mblk_t *);
extern void	LipmxBogusIoctl(queue_t *, mblk_t *);

extern void	LipmxMctlGetMaxSDU(queue_t *, mblk_t *);
extern void	LipmxMctlGetIntNetNode(queue_t *, mblk_t *);
extern void	LipmxBogusMctl(queue_t *, mblk_t *);
extern int	LipmxCopyLanNode(uint32, uint8 *);
extern int	LipmxCopyLanNet(uint32, uint32 *);

/*
**	Statistics
*/
extern IpxLanStats_t lipmxStats;			/* Statistics Structure */

#endif	/* _NET_NW_IPX_LIPMX_LIPMX_IOCTLS_H */

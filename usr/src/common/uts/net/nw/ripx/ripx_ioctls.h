/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ripx/ripx_ioctls.h	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_RIPX_RIPX_IOCTLS_H  /* wrapper symbol for kernel use */
#define _NET_NW_RIPX_RIPX_IOCTLS_H  /* subject to change without notice */

#ident	"$Id: ripx_ioctls.h,v 1.5 1994/02/23 19:23:45 vtag Exp $"

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

extern void	RipIBogusIoctl(queue_t *, mblk_t *);
extern void	RipIDumpHashTable(queue_t *, mblk_t *);
extern void	RipIGetHashTableStats(queue_t *, mblk_t *);
extern void	RipIGetRouteTableHashSize(queue_t *, mblk_t *);
extern void	RipIocAck(queue_t *, mblk_t *, uint32);
extern void	RipIocNegAck(queue_t *, mblk_t *, int);
extern void	RIPSetRouterType(queue_t *, mblk_t *);

#endif /* _NET_NW_RIPX_RIPX_IOCTLS_H */

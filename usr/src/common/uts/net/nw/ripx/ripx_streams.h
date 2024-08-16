/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ripx/ripx_streams.h	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_RIPX_RIPX_STREAMS_H  /* wrapper symbol for kernel use */
#define _NET_NW_RIPX_RIPX_STREAMS_H  /* subject to change without notice */

#ident	"$Id: ripx_streams.h,v 1.5 1994/03/16 17:57:31 vtag Exp $"

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
#include <io/ddi.h>
#else
#include "nwcommon.h"
#endif /* _KERNEL_HEADERS */


extern int  ripxchain(queue_t *);
extern int  ripxinit(void);
extern int  ripxfini(void);
extern void	NotifySapNetDead(uint32);
extern int	ripxAllocStreamsLocks(void);
extern void	ripxDeallocStreamsLocks(void);

#define RIPX_INFO ripxinfo

#define IS_PRIV(queue)	((uint32)(queue->q_ptr) & PRIV_MASK)
#define MAX_MINORS		(sizeof(q->q_ptr) * 8)
#define PRIV_MASK		1
#define MINOR_MASK		(~(uint32)1)

/*
**	Heirarchies for Streams Locks
*/
#define	DEVICE_LOCK	31
#define SAPQ_LOCK	32

#endif /* _NET_NW_RIPX_RIPX_STREAMS_H */

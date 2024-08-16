/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/lipmx/rripx.h	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_IPX_LIPMX_RRIPX_H  /* wrapper symbol for kernel use */
#define _NET_NW_IPX_LIPMX_RRIPX_H  /* subject to change without notice */

#ident	"$Id: rripx.h,v 1.4 1994/02/23 19:22:59 vtag Exp $"

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

/* rripx.h
 *
 * rripx.c implements IPX's share of the Replaceable Router
 * interface. This header exposes to IPX the portions of that
 * implementation (not seen through rrouter.h) that IPX needs
 * to see.
 */

#ifdef _KERNEL_HEADERS
#include <net/nw/ipx/lipmx/lipmx.h>
#include <net/nw/rrouter.h>
#else
#include "lipmx.h"
#include "rrouter.h"
#endif /* _KERNEL_HEADERS */

extern RROUTERMethods_t	RROUTER;

#endif	/* _NET_NW_IPX_LIPMX_RRIPX_H */

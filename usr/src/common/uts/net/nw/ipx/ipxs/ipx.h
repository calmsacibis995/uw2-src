/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/ipxs/ipx.h	1.7"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW4U_IPX_IPXS_IPX_H  /* wrapper symbol for kernel use */
#define _NET_NW4U_IPX_IPXS_IPX_H  /* subject to change without notice */

#ident	"$Id: ipx.h,v 1.10 1994/05/16 21:37:42 meb Exp $"

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
 * ipx   - Novell Streams IPX Driver
 */

#ifdef _KERNEL_HEADERS
#include <net/nw/nwcommon.h>
#include <net/tihdr.h>
#include <net/tiuser.h>
#include <svc/time.h>
#include <net/dlpi.h>
#include <net/nw/sap.h>
#include <net/nw/ipx_app.h>
#include <net/nw/ripx_app.h>
#include <net/nw/ipx_lipmx.h>
#include <net/nw/spxipx.h>
#else
#include "nwcommon.h"
#include <sys/tihdr.h>
#include <sys/tiuser.h>
#include <sys/time.h>
#include <sys/dlpi.h>
#include "sys/sap.h"
#include "sys/ipx_app.h"
#include "sys/ripx_app.h"
#include "ipx_lipmx.h"
#include "spxipx.h"
#endif /* _KERNEL_HEADERS */

#ifdef NTR_TRACING
#define	NTR_ModMask		NTRM_ipx
#endif

#define IPX_MIN_EPHEMERAL_SOCKET 0x4000
#define IPX_MAX_EPHEMERAL_SOCKET 0x7FFF
#define IPX_EPHEMERAL_SOCKET_RANGE  (IPX_MAX_EPHEMERAL_SOCKET - \
									IPX_MIN_EPHEMERAL_SOCKET)

#define IPX_MIN_SOCKETS	10	/* dev0, spx, ncp, diag x 3, ... */
#define IPX_MAX_SOCKETS	4096

/* tpi defines */
#define IPX_ETSDU_SIZE -2
#define IPX_CDATA_SIZE -2
#define IPX_DDATA_SIZE -2
#define IPX_OPT_SIZE 3

/*
**	Checksum flags
*/
#define CHKSUM_VERIFY   1
#define CHKSUM_CALC     2

#define IPXCMP16    IPXCMPSOCK

extern uint32	ipxDevices[];
extern int		ipxMaxDevices;

/*
 * Some sources say this should be 1 not 0.
 * We use 0 because that is what native 3.11
 * does.
 */
#define TRANSPORT_CONTROL 0

/*
**	From address of socket in net order, compute hash
*/
#define HASH_MASK 0xFF
#define SOCKETHASH(p) ((((uint8 *)p)[0] + ((uint8 *)p)[1]) & HASH_MASK)

/* 
 * Private data structure for each open stream to the ipx driver
 *	This data structure is at RQ(q)->q_ptr and must fit in one long.
 *	WQ(q)->q_ptr contains the minor device number.
 */
typedef struct ipxPDS {
	int8			priv;		/* mask bits */
	uint8			pad;		/* unused */
	uint16			socket;		/* first bound socket number, net order */
} ipxPDS_t;

/*
**	Flag bits for the priv mask
*/
#define PRIV_USER	(1<<0)	/* Privileged user */
#define SET_SOCKET	(1<<1)	/* Set socket used */
#define BIND_SOCKET	(1<<2)	/* Bind socket used (possible multiple binds) */
#define TLI_SOCKET	(1<<3)	/* TLI socket used */
#define KNL_SOCKET	(1<<4)	/* Kernel bound socket (possible multiple binds) */

typedef struct ipxSocket {
	queue_t			*qptr;	/* pointer to read queue for this socket */
	uint16			 socket;/* ipx socket assigned to this stream in net order*/
	uint16			 device;/* minor device or 0xffff if control device */
	struct ipxSocket *hlink;/* pointer to next socket struct at this hash */
	struct ipxSocket *qlink;/* pointer to next socket struct for this queue */
	lock_t			 *qlock;/* queue link lock */
	atomic_int_t	 qcount;/* count of putnext calls active on this queue */
	uint16			 ref;	/* reference count */
} ipxSocket_t;

/*
**	Define Hash Table Structure
*/
typedef struct {
	ipxSocket_t	*hlink;		/* Link to first entry in hash bucket */
	lock_t		*hlock;		/* Hash bucket lock */
} ipxHash_t;

/*
**	Functions definitions that must be known across .c files
*/
int	ipxinit(void);	/* Driver load initializaton */
int	ipxfini(void);	/* Driver unload termination */

#endif /* _NET_NW4U_IPX_IPXS_IPX_H */

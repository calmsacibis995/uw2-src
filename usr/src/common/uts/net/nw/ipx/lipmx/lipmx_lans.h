/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/ipx/lipmx/lipmx_lans.h	1.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_IPX_LIPMX_LIPMX_LANS_H  /* wrapper symbol for kernel use */
#define _NET_NW_IPX_LIPMX_LIPMX_LANS_H  /* subject to change without notice */

#ident	"$Id: lipmx_lans.h,v 1.2 1994/02/18 15:21:40 vtag Exp $"

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

typedef struct Lan {
	uint32		lan;			/* lan table index of this connected lan */
	int			l_index;		/* unique link index for I_LINK/UNLINK */
	queue_t		*qbot;			/* bottom write queue leading out Lan */
	atomic_int_t qcount;		/* count of putnext calls active on this que */
	void		*rrLanKey;		/* token to access router methods */
	uint32		state;			/* Ipx state */
	uint32		ipxNet;			/* connected lan net address */
	uint8		ipxNode[6];		/* connected lan node address */
	dlInfo_t	dlInfo;			/* data link layer info */
	mblk_t		*directedPaced;	/* ptr to private directed-paced link list */
	mblk_t		*bcastPaced;	/* ptr to private broadcast-paced link list */
	ripSapInfo_t ripSapInfo;	/* RIP and SAP lan info */
	time_t		lastPacedSend;	/* when last paced directed pkt sent */
	time_t		paceTicks;
	toid_t		sendId;			/* timeout id for paced directed sends */
	lock_t		*lanLock;		/* Lock for this lan structure */
} Lan_t;

#endif /* _NET_NW_IPX_LIPMX_LIPMX_LANS_H */

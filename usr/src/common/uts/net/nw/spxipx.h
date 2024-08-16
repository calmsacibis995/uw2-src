/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/nw/spxipx.h	1.5"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_NW_SPXIPX_H  /* wrapper symbol for kernel use */
#define _NET_NW_SPXIPX_H  /* subject to change without notice */

#ident	"$Id: spxipx.h,v 1.3 1994/09/28 16:36:06 vtag Exp $"

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
 * SPX to IPX M_CTLs
 */

#define	IPX_O_BIND_SOCKET		( ( 'i' << 8 ) | 252 ) /* Non XTI bind req */
#define GET_IPX_INT_NET_NODE	( ( 'i' << 8 ) | 253 ) 
#define SPX_GET_IPX_MAX_SDU		( ( 's' << 8 ) | 254 ) 

typedef struct spxGetIpxMaxSDU {
	uint32					network;
	uint32					maxSDU;
	caddr_t					conEntry;
} spxGetIpxMaxSDU_t;

typedef struct ipxInternalNetNode {
	uint32	net;
	uint8	node[IPX_NODE_SIZE];
} ipxInternalNetNode_t;

typedef union {
	struct iocblk	iocblk;
	struct {
		int		cmd;
		union {
			spxGetIpxMaxSDU_t	spxGetIpxMaxSDU;
			ipxInternalNetNode_t	ipxInternalNetNode;
		} u_mctl;
	} mctlblk;
} ipxMctl_t;

#endif	/* _NET_NW_SPXIPX_H */

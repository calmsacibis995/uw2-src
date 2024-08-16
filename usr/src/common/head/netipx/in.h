/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:netipx/in.h	1.2"
#ident "$Header: /SRCS/esmp/usr/src/nw/head/netipx/in.h,v 1.1.1.1 1993/10/11 20:27:57 ram Exp $"

/*
	This source has been modified by Scott Harrison of Novell Inc. to support
    the IPX address family.  Some, but not all of the modifictation are
    surrounded by an #if	defined(NOVELL_NUC_MOD).
*/
#ifndef	_IN_H
#define	_IN_H

typedef struct ipx_addr_s
    {
    u_char
	ipxaddr_net[4],
	ipxaddr_node[6];
    }	ipx_addr_t;

typedef struct sockaddr_ipx_s
    {
    short
	sipx_family;
    ushort
	sipx_port;
    ipx_addr_t
	sipx_addr;
    char
	sipx_zero[2];
    }	sockaddr_ipx_t;

#endif	/* _IN_H */

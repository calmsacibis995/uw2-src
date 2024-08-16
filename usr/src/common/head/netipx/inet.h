/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:netipx/inet.h	1.2"
#ident "$Header: /SRCS/esmp/usr/src/nw/head/netipx/inet.h,v 1.1.1.1 1993/10/11 20:27:57 ram Exp $"

/*
	This source has been modified by Scott Harrison of Novell Inc. to support
    the IPX address family.  Some, but not all of the modifictation are
    surrounded by an #if	defined(NOVELL_NUC_MOD).
*/
/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * External definitions for
 * functions in inet(3N)
 */

#ifndef _INET_H
#define _INET_H

ipx_addr_t ipx_addr();
char	*ipx_ntoa();
unsigned long ipx_network();

#endif	/* _INET_H */

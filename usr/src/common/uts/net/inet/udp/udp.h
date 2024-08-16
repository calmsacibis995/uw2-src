/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_INET_UDP_UDP_H	/* wrapper symbol for kernel use */
#define _NET_INET_UDP_UDP_H	/* subject to change without notice */

#ident	"@(#)kern:net/inet/udp/udp.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ifdef _KERNEL_HEADERS

#include <net/inet/udp/udp_f.h>		/* PORTABILITY */

#elif defined(_KERNEL)

#include <netinet/udp_f.h>		/* PORTABILITY */

#else /* user */

#include <netinet/udp_f.h>		/* PORTABILITY */

#endif /* _KERNEL_HEADERS */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_INET_UDP_UDP_H */

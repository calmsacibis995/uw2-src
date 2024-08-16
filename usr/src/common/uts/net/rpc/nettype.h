/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_NETTYPE_H	/* wrapper symbol for kernel use */
#define _NET_RPC_NETTYPE_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/nettype.h	1.20"
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

/*
 *	nettype.h, network type definitions.
 *	All for the topmost layer of rpc
 *
 */

#ifdef _KERNEL_HEADERS

#include <net/netconfig.h>	 /* REQUIRED */

#elif defined(_KERNEL)

#include <sys/netconfig.h>	 /* REQUIRED */

#else

#include <netconfig.h> 		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#define _RPC_NONE		0
#define _RPC_NETPATH		1
#define _RPC_VISIBLE		2
#define _RPC_CIRCUIT_V		3
#define _RPC_DATAGRAM_V		4
#define _RPC_CIRCUIT_N		5
#define _RPC_DATAGRAM_N		6
#define _RPC_TCP		7
#define _RPC_UDP		8

#ifdef __STDC__

extern void 			*_rpc_setconf(char *);
extern void 			_rpc_endconf(void *);
extern struct netconfig		*_rpc_getconf(void *);
extern struct netconfig		*_rpc_getconfip(char *);

#else

extern	void			*_rpc_setconf();
extern	void 			_rpc_endconf();
extern	struct netconfig	*_rpc_getconf();
extern	struct netconfig	*_rpc_getconfip();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_NETTYPE_H */

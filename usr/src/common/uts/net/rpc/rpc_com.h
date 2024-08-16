/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_RPC_COM_H	/* wrapper symbol for kernel use */
#define _NET_RPC_RPC_COM_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/rpc_com.h	1.15"
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
 *	rpc_com.h, common definitions for both the server and client
 *	side. All for the topmost layer of rpc.
 */ 

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * File descriptor to be used on xxx_create calls to get default descriptor
 */
#define	RPC_ANYSOCK		-1
#define RPC_ANYFD		RPC_ANYSOCK	

/*
 * The max size of the transport, if the size cannot be determined
 * by other means.
 */
#define	RPC_MAXDATASIZE 9000
#define	RPC_MAXADDRSIZE 1024

#ifdef __STDC__

extern	u_int			_rpc_get_t_size (int, long);
extern	u_int			_rpc_get_a_size (long);
extern	int			_rpc_dtbsize (void);
extern	struct netconfig	*_rpcgettp(int);
extern	int			_rpc_get_default_domain(char **);

#else

extern	u_int			_rpc_get_t_size ();
extern	u_int			_rpc_get_a_size ();
extern	int			_rpc_dtbsize ();
extern	struct netconfig	*_rpcgettp();
extern	int			_rpc_get_default_domain();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_RPC_COM_H */

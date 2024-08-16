/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_CLNT_SOC_H	/* wrapper symbol for kernel use */
#define _NET_RPC_CLNT_SOC_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/clnt_soc.h	1.16"
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
 *	clnt_soc.h, client side remote procedure call interface
 *	using berkeley sockets, present only for compatibility
 *	with original rpc interface.
 */

#ifdef _KERNEL_HEADERS

#include <net/rpc/clnt.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <rpc/clnt.h>		/* REQUIRED */

#else

#include <sys/socket.h>		/* SVR4.0COMPAT */
#include <netinet/in.h>		/* SVR4.0COMPAT */
#include <rpc/xdr.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * rpc imposed limit on udp msg size
 */
#define UDPMSGSIZE		8800

#ifdef __STDC__

extern	int	callrpc (char *, u_long, u_long, u_long, xdrproc_t, char *,
			xdrproc_t, char *);

#else

extern	int	callrpc ();

#endif

/*
 * tcp, udp and memory based rpc.
 */

#ifdef __STDC__

extern	CLIENT		*clnttcp_create (struct sockaddr_in *, u_long,
				u_long, int *, u_int, u_int);
extern	CLIENT		*clntudp_create (struct sockaddr_in *, u_long,
				u_long, struct timeval, int *);
extern	CLIENT		*clntudp_bufcreate(struct sockaddr_in *, u_long,
				u_long, struct timeval, int *, u_int, u_int);
extern	CLIENT		*clntraw_create (u_long, u_long);

#else

extern	CLIENT		*clnttcp_create ();
extern	CLIENT		*clntudp_create ();
extern	CLIENT		*clntudp_bufcreate ();
extern	CLIENT		*clntraw_create ();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_CLNT_SOC_H */

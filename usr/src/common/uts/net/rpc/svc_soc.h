/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_SVC_SOC_H	/* wrapper symbol for kernel use */
#define _NET_RPC_SVC_SOC_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/svc_soc.h	1.15"
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
 *	svc_soc.h, server-side remote procedure call interface
 *	using berkeley sockets, present only for compatibility
 *	with original interface.
 */

#ifdef _KERNEL_HEADERS

#include <net/rpc/types.h> 	/* REQUIRED */
#include <net/rpc/svc.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <rpc/types.h> 		/* REQUIRED */
#include <rpc/svc.h>		/* REQUIRED */

#else

#include <sys/socket.h> 	/* SVR4.0COMPAT */
#include <netinet/in.h>		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * approved way of getting address of caller
 */
#define svc_getcaller(x) ((struct sockaddr_in *)(x)->xp_rtaddr.buf)

/*
 * Service registration
 *
 * svc_register(xprt, prog, vers, dispatch, protocol)
 *	SVCXPRT *xprt;
 *	u_long prog;
 *	u_long vers;
 *	void (*dispatch)();
 *	int protocol;
 */
#ifdef __STDC__

extern	bool_t		svc_register (SVCXPRT *, u_long, u_long,
			void (*)(struct svc_req *, SVCXPRT *), int);
#else

extern	bool_t		svc_register ();

#endif

/*
 * Service un-registration
 *
 * svc_unregister(prog, vers)
 *	u_long prog;
 *	u_long vers;
 */

#ifdef __STDC__

extern	void		svc_unregister (u_long, u_long);

#else

extern	void		svc_unregister ();

#endif

/*
 * memory based rpc for testing and timing.
 */

#ifdef __STDC__

extern	SVCXPRT		*svcraw_create (void);

#else

extern	SVCXPRT		*svcraw_create ();

#endif

/*
 * udp based rpc, for compatibility reasons
 */

#ifdef __STDC__

extern	SVCXPRT		*svcudp_create (int);
extern	SVCXPRT		*svcudp_bufcreate (int, u_int, u_int);

#else

extern	SVCXPRT		*svcudp_create ();
extern	SVCXPRT		*svcudp_bufcreate ();

#endif

/*
 * tcp based rpc, for compatibility reasons
 */

#ifdef __STDC__

extern	SVCXPRT		*svctcp_create (int, u_int, u_int);
extern	SVCXPRT		*svcfd_create (int, u_int, u_int);

#else

extern	SVCXPRT		*svctcp_create ();
extern	SVCXPRT		*svcfd_create ();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_SVC_SOC_H */

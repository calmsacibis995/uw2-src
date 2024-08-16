/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_PMAP_CLNT_H	/* wrapper symbol for kernel use */
#define _NET_RPC_PMAP_CLNT_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/pmap_clnt.h	1.15"
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
 *	pmap_clnt.h, routines to get to the portmapper's services.
 *	present for compatibility, rpcbind has replaced the
 *	portmapper.
 *	
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/rpc/types.h>	/* REQUIRED */
#include <net/rpc/clnt.h>	/* REQUIRED */
#include <net/rpc/xdr.h>	/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <rpc/types.h>		/* REQUIRED */
#include <rpc/clnt.h>		/* REQUIRED */
#include <rpc/xdr.h>		/* REQUIRED */

#else

#include <sys/types.h>		/* REQUIRED */
#include <rpc/types.h>		/* REQUIRED */
#include <rpc/clnt.h>		/* REQUIRED */
#include <rpc/xdr.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 *	success = pmap_set(program, version, protocol, port);
 *	Register a procedure with the portmapper
 *
 *	success = pmap_unset(program, version);
 *	Unregister a procedure with the portmapper
 *
 *	port = pmap_getport(address, program, version, protocol);
 *	Get a port for a procedure from the portmapper
 *
 *	head = pmap_getmaps(address);
 *	Get all the procedures registered with the portmapper
 *
 *	clnt_stat = pmap_rmtcall(address, program, version, procedure,
 *		xdrargs, argsp, xdrres, resp, tout, port_ptr)
 *	Make a remote procedure call. Works for udp only.
 *
 * 	clnt_stat = clnt_broadcast(program, version, procedure,
 *		xdrargs, argsp,	xdrres, resp, eachresult)
 *	Broadcast a remote procedure call. Like pmap_rmtcall(), except
 *	the call is broadcasted to all locally connected nets. For each
 *	valid response received, the procedure eachresult is called.
 *
 *	Its form is:
 *	done = eachresult(resp, raddr)
 *	where resp points to the results of the call and raddr is the
 *	address if the responder to the broadcast.
 *
 */

#ifdef __STDC__

extern	bool_t			pmap_set(u_long, u_long, int, u_short);
extern	bool_t			pmap_unset(u_long, u_long);
extern	struct	pmaplist	*pmap_getmaps(struct sockaddr_in *);
extern	enum clnt_stat		pmap_rmtcall(struct sockaddr_in *, u_long,
					     u_long, u_long, xdrproc_t, caddr_t,
					     xdrproc_t, caddr_t, struct timeval,
					     u_long *);
extern	u_short			pmap_getport(struct sockaddr_in *, u_long,
					u_long, u_int);
extern	enum clnt_stat		clnt_broadcast(u_long, u_long, u_long,
					xdrproc_t, caddr_t, xdrproc_t,
					caddr_t, resultproc_t);

#else

extern	bool_t			pmap_set();
extern	bool_t			pmap_unset();
extern	enum clnt_stat		pmap_rmtcall();
extern	struct pmaplist		*pmap_getmaps();
extern	u_short			pmap_getport();
extern	enum clnt_stat		clnt_broadcast();

#endif /* __STDC__ */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_PMAP_CLNT_H */

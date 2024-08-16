/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_PMAP_RMT_H	/* wrapper symbol for kernel use */
#define _NET_RPC_PMAP_RMT_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/pmap_rmt.h	1.13"
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
 *	pmap_rmt.h, structures and XDR routines for parameters to
 *	and replies from the portmapper remote-call-service.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/rpc/xdr.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <rpc/xdr.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

struct rmtcallargs {
	u_long			prog, vers, proc, arglen;
	caddr_t			args_ptr;
	xdrproc_t		xdr_args;
};

extern	bool_t			xdr_rmtcallargs ();

struct rmtcallres {
	u_long			*port_ptr;
	u_long			resultslen;
	caddr_t			results_ptr;
	xdrproc_t		xdr_results;
};

typedef	struct rmtcallres	rmtcallres;
extern	bool_t			xdr_rmtcallres ();


#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_PMAP_RMT_H */

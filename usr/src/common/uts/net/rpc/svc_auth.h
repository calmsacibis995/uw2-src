/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_SVC_AUTH_H	/* wrapper symbol for kernel use */
#define _NET_RPC_SVC_AUTH_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/svc_auth.h	1.13"
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
 *	svc_auth.h, service side of rpc authentication.
 */

#ifdef _KERNEL_HEADERS

#include <net/rpc/svc.h>		/* REQUIRED */
#include <net/rpc/rpc_msg.h>		/* REQUIRED */

#elif defined(_KERNEL) 

#include <rpc/svc.h>			/* REQUIRED */
#include <rpc/rpc_msg.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef __STDC__

extern	enum auth_stat	_authenticate (struct svc_req *, struct rpc_msg *);

#else

extern	enum auth_stat	_authenticate ();

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_SVC_AUTH_H */

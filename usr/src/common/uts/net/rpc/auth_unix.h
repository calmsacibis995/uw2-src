/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_AUTH_UNIX_H	/* wrapper symbol for kernel use */
#define _NET_RPC_AUTH_UNIX_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/auth_unix.h	1.11"
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
 *	auth_unix.h, protocol for unix style authentication
 *	parameters for rpc. Now obsolete. Users should
 *	switch to auth_sys.h.
 */

#ifdef _KERNEL_HEADERS

#include <net/rpc/auth_sys.h>		/* REQUIRED */

#elif defined(_KERNEL)

#include <rpc/auth_sys.h>		/* REQUIRED */

#else

#include <rpc/auth_sys.h> 		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_AUTH_UNIX_H */

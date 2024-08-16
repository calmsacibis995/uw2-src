/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_TYPES_H	/* wrapper symbol for kernel use */
#define _NET_RPC_TYPES_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/types.h	1.19"
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

#ifdef _NSL_RPC_ABI

/*
 * For internal use only when building the libnsl RPC routines
 */

/* libnsl's internal thread-safe versions of syslog functions */
#define syslog		_nsl_syslog
#define vsyslog		_nsl_vsyslog
#define openlog		_nsl_openlog
#define closelog	_nsl_closelog
#define setlogmask	_nsl_setlogmask

/* The libc versions of these functions are thread-safe. */
#define select		_abi_select
#define gettimeofday	_abi_gettimeofday
#define getgrent	_abi_getgrent
#define endgrent	_abi_endgrent
#define setgrent	_abi_setgrent

#endif /* _NSL_RPC_ABI */

/*
 *	types.h, rpc additions to <util/types.h>
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <mem/kmem.h> 		/* REQUIRED */

#elif defined(_KERNEL)

#include <sys/types.h>		/* REQUIRED */
#include <sys/kmem.h> 		/* REQUIRED */

#else

#include <sys/types.h>		/* SVR4.0COMPAT */
#include <sys/time.h> 		/* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

#define		bool_t		int
#define		enum_t		int
#define		__dontcare__	-1

#ifndef FALSE
#	define	FALSE	(0)
#endif

#ifndef TRUE
#	define	TRUE	(1)
#endif

#ifndef NULL
#	define NULL 0
#endif

#ifndef _KERNEL

#define	mem_alloc(bsize)	malloc(bsize)
#define mem_free(ptr, bsize)	free(ptr)

#else

#ifdef DEBUG

extern int	rpc_log();
extern int	rpclog;

#define		RPCLOG(A, B, C) ((void)((rpclog) && rpc_log((A), (B), (C))))

#else

#define		RPCLOG(A, B, C)

#endif

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_TYPES_H */

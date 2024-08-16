/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_TOKEN_H	/* wrapper symbol for kernel use */
#define _NET_RPC_TOKEN_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/token.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	token.h, definitions for token types
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/xti.h>		/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <sys/xti.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */

/*
 * token types, for get_remote_token call
 */
#define PRIVS_T		1	/* privilege vector */
#define SENS_T		2	/* sensitivity token */
#define INFO_T		3	/* information token */
#define INTEG_T		4	/* integrity token */
#define NCS_T		5	/* national caveat */
#define ACL_T		6	/* access control list token */

/*
 * the token data type
 */
typedef	u_long		s_token;

#ifdef _KERNEL

/*
 * the token service external interface
 */
#ifdef __STDC__

extern	s_token		get_remote_token(struct netbuf *, u_int, caddr_t,
				u_int);
extern	u_int		map_local_token(s_token, u_int, caddr_t, u_int);

#endif

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_TOKEN_H */

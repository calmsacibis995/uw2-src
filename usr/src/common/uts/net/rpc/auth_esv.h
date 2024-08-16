/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_AUTH_ESV_H	/* wrapper symbol for kernel use */
#define _NET_RPC_AUTH_ESV_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/auth_esv.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	auth_esv.h, protocol for secure style authentication parameters
 *	for kernel rpc. This protocol is really an extension of unix or
 *	kernel style auth. The security specific attributes are also
 *	sent back and forth by the client and server. It is very weak.
 *	The client uses no encryption for its credentials and only sends
 *	null verifiers. The server sends backs null verifiers or optionally
 *	a verifier that suggests a new short hand for the credentials.
 *
 *	The server does no validation of the client's credentials and vice
 *	versa. Authentication fails only when there is an error during
 *	serialization and deserialization of authentication information.
 */

#ifdef _KERNEL_HEADERS

#include <util/types.h>		/* REQUIRED */
#include <net/rpc/types.h> 	/* REQUIRED */
#include <net/rpc/auth.h>	/* REQUIRED */
#include <net/rpc/token.h>	/* REQUIRED */

#elif defined(_KERNEL) 

#include <sys/types.h>		/* REQUIRED */
#include <rpc/types.h> 		/* REQUIRED */
#include <rpc/auth.h>		/* REQUIRED */
#include <rpc/token.h>		/* REQUIRED */

#endif /* _KERNEL_HEADERS */


#ifdef	_KERNEL

/*
 * The machine name is part of a credential.
 * It may not exceed 255 bytes
 */
#define	MAX_ESVMACH_NAME	255

/*
 * groups compose part of a credential.
 * there may not be more than 24 of them.
 */
#define ESV_NGRPS		24

/*
 * base (smallest) credentials size in esv
 */
#define	BASE_ESVCREDSZ		11

/*
 * "esv" (secure) style credentials, extention of kern or unix or sys style
 */
struct authesv_parms {
	u_long	 auc_stamp;	/* arbitrary timestamp */
	char	*auc_machname;	/* machine name */
	uid_t	 auc_uid;	/* user id */
	gid_t	 auc_gid;	/* group id */
	u_int	 auc_len;	/* total length of groups */
	gid_t	*auc_gids;	/* groups */
	u_long	 auc_aid;	/* additional id, pid */
	s_token	 auc_privs;	/* privilege vector */
	s_token	 auc_sens;	/* sensitivity label */
	s_token	 auc_info;	/* information label */
	s_token	 auc_integ;	/* integrity token */
	s_token	 auc_ncs;	/* nationality caveat */
};
extern bool_t xdr_authesv_parms(XDR *, struct authesv_parms *);

#endif

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_AUTH_ESV_H */

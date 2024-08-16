/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _NET_RPC_RPC_H	/* wrapper symbol for kernel use */
#define _NET_RPC_RPC_H	/* subject to change without notice */

#ident	"@(#)kern:net/rpc/rpc.h	1.12"
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
 *	rpc.h, just includes the billions of rpc header files necessary to 
 *	do remote procedure calling.
 *
 */

#ifdef _KERNEL_HEADERS

#include <net/rpc/types.h> 
#include <net/xti.h>
#include <fs/fcntl.h>	
#include <net/inet/in.h>	
#include <net/ktli/t_kuser.h>	
#include <net/rpc/xdr.h>	
#include <net/rpc/auth.h>	
#include <net/rpc/clnt.h>	
#include <net/rpc/rpc_msg.h>	
#include <net/rpc/auth_sys.h>	
#include <net/rpc/auth_des.h>	
#include <net/rpc/auth_esv.h>
#include <net/rpc/svc.h>	
#include <net/rpc/svc_auth.h>	

#elif defined(_KERNEL)

#include <rpc/types.h> 
#include <sys/xti.h>	
#include <sys/fcntl.h>	
#include <netinet/in.h>	
#include <sys/t_kuser.h>	
#include <rpc/xdr.h>	
#include <rpc/auth.h>	
#include <rpc/clnt.h>	
#include <rpc/rpc_msg.h>	
#include <rpc/auth_sys.h>	
#include <rpc/auth_des.h>	
#include <rpc/auth_esv.h>
#include <rpc/svc.h>	
#include <rpc/svc_auth.h>	

#else	/* _KERNEL */

#include <rpc/types.h>
#include <xti.h>
#include <fcntl.h>
#include <memory.h>
#include <rpc/xdr.h>		/* generic (de)serializer */
#include <rpc/auth.h>		/* generic authenticator (client side) */
#include <rpc/clnt.h>		/* generic client side rpc */
#include <rpc/rpc_msg.h>	/* protocol for rpc messages */
#include <rpc/auth_sys.h>	/* protocol for unix style cred */
#include <rpc/auth_des.h>	/* protocol for des style cred */
#include <rpc/svc.h>		/* service manager and multiplexer */
#include <rpc/svc_auth.h>	/* service side authenticator */
#include <rpc/rpcb_clnt.h>	/* rpcbind interface functions */

#endif /* _KERNEL_HEADERS */

#if defined(__cplusplus)
	}
#endif

#endif /* _NET_RPC_RPC_H */

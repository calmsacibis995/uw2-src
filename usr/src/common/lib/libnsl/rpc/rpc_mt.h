/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpc_mt.h	1.1.2.4"
#ident	"$Header: $"

/*
 * rpc_mt.h
 */

#include <mt.h>

#ifdef _REENTRANT

struct _rpc_tsd {
	void	*clnt_error_p;
	void	*hostent_p;
	void	*rpcent_p;
	void	*inet_p;
	void	*createerr_p;
	void	*call_p;
};

#define RPC_KEYTBL_SIZE    ( sizeof (struct _rpc_tsd) / sizeof (void *) )

extern void _free_rpc_keytbl		(void *);
extern void _free_rpc_clnt_error	(void *);
extern void _free_rpc_hostent		(void *);
extern void _free_rpc_rpcent		(void *);
extern void _free_rpc_inet		(void *);
extern void _free_rpc_createerr		(void *);
extern void _free_rpc_call		(void *);

extern THREAD_KEY_T __rpc_key;
extern MUTEX_T __rpc_lock;
extern MUTEX_T __list_lock;
extern MUTEX_T __rpcbind_lock;
extern MUTEX_T __keyserv_lock;
extern MUTEX_T __svc_lock;
extern MUTEX_T __authdes_lock;
extern MUTEX_T __bindresvport_lock;
extern MUTEX_T __nsl_syslog_lock;

#endif /* _REENTRANT */


/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpc_mt.c	1.2.2.4"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * rpc_mt.c
 * This file includes all multi-threading related data and 
 * functions, as well as the definition of rpc_createerr()
 */

#include <stdlib.h>
#include "rpc/rpc.h"
#include "trace.h"
#include "rpc_mt.h"

/*
 * rpc_createerr is #defined as *(_rpc_createerr())
 * in rpc/clnt.h but here we don't want that.
 */
#undef rpc_createerr
extern rpc_createerr_t rpc_createerr;
/*
 * Storage must exist for get_rpc_createerr() and _rpc_createerr() to
 * access when memory is low.
 */
static const rpc_createerr_t __rpc_createerr = { RPC_SYSTEMERROR }; 

#ifdef _REENTRANT

MUTEX_T __rpc_lock;
MUTEX_T __list_lock;
MUTEX_T __rpcbind_lock;
MUTEX_T __keyserv_lock;
MUTEX_T __svc_lock;
MUTEX_T __authdes_lock;
MUTEX_T __bindresvport_lock;
MUTEX_T __nsl_syslog_lock;

THREAD_KEY_T __rpc_key;

#endif /* _REENTRANT */

void
_rpc_init()
{
#ifdef _REENTRANT

	/* Initialize locks */

	MUTEX_INIT(&__rpc_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&__list_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&__rpcbind_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&__keyserv_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&__svc_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&__authdes_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&__bindresvport_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&__nsl_syslog_lock, USYNC_THREAD, NULL);

	/* create keys for per-thread storage */

	THR_KEYCREATE(&__rpc_key, &_free_rpc_keytbl);

#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rpc_keytbl(void *t)
{
	struct _rpc_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _rpc_tsd *)t;

	_free_rpc_clnt_error	(key_tbl->clnt_error_p);
	_free_rpc_hostent	(key_tbl->hostent_p);
	_free_rpc_rpcent	(key_tbl->rpcent_p);
	_free_rpc_inet		(key_tbl->inet_p);
	_free_rpc_createerr	(key_tbl->createerr_p);
	_free_rpc_call		(key_tbl->call_p);
	free		(t);
}

#endif /* _REENTRANT */

rpc_createerr_t
get_rpc_createerr()
{
#ifdef _REENTRANT
	struct _rpc_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD)
		return (rpc_createerr);

	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _rpc_tsd *)
		  _mt_get_thr_specific_storage(__rpc_key, RPC_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->createerr_p != NULL)
		return(*(rpc_createerr_t *)key_tbl->createerr_p);
	return (__rpc_createerr);
#else
	return (rpc_createerr);
#endif /* _REENTRANT */
}

int
set_rpc_createerr(errstruct)
	rpc_createerr_t errstruct;
{
#ifdef _REENTRANT
	struct _rpc_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
		rpc_createerr = errstruct;
		return 0;
	}
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _rpc_tsd *)
		  _mt_get_thr_specific_storage(__rpc_key, RPC_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return -1;
	if (key_tbl->createerr_p == NULL) 
		key_tbl->createerr_p = calloc(1, sizeof(rpc_createerr_t));
	if (key_tbl->createerr_p == NULL) return -1;
	*(rpc_createerr_t *)key_tbl->createerr_p = errstruct;
#else
	rpc_createerr = errstruct;
#endif /* _REENTRANT */
	return 0;
}

const rpc_createerr_t *
_rpc_createerr()
{
#ifdef _REENTRANT
	struct _rpc_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD)
		return (&rpc_createerr);

	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _rpc_tsd *)
		  _mt_get_thr_specific_storage(__rpc_key, RPC_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->createerr_p != NULL)
		return((rpc_createerr_t *)key_tbl->createerr_p);
	return (&__rpc_createerr);
#else
	return (&rpc_createerr);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rpc_createerr(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

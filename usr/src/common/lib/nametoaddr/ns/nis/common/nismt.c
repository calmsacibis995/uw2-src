/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/common/nismt.c	1.2"
#ident  "$Header: $"

/*
 * libsock_mt.c
 * This file includes the _init() function for initializing libsocket.
 */
#include <stdlib.h>
#include <netdb.h>
#ifdef _REENTRANT
#include "nis_mt.h"

THREAD_KEY_T	_nis_key;
MUTEX_T         _nis_domain_lock;

#endif /* _REENTRANT */

void
_nis_init()
{
#ifdef _REENTRANT


	/* create keys for per-thread storage */

	THR_KEYCREATE(&_nis_key, &_free_nis_keytbl);

	MUTEX_INIT(&_nis_domain_lock, USYNC_THREAD, NULL);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_nis_keytbl(void *t)
{
	struct _nis_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _nis_tsd *)t;

	_free_nis_net_info		(key_tbl->net_info_p);
	_free_nis_proto_info	(key_tbl->proto_info_p);
	_free_nis_serv_info		(key_tbl->serv_info_p);
	_free_nis_host_info		(key_tbl->host_info_p);
	_free_nis_rpc_info		(key_tbl->rpc_info_p);
	_free_nis_errno			(key_tbl->nis_errno_p);
	_free_nis_pwd_info		(key_tbl->pwd_info_p);
	_free_nis_gr_info		(key_tbl->gr_info_p);
	free			(t);
}

#endif /* _REENTRANT */

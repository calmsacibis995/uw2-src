/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:common/lib/libsocket/socket/libsock_mt.c	1.3"
#ident	"$Header: $"

/*
 * libsock_mt.c
 * This file includes the _init() function for initializing libsocket.
 */
#include <stdlib.h>
#include <netdb.h>
#include "../libsock_mt.h"

#ifdef _REENTRANT

MUTEX_T		_s_domain_lock;
MUTEX_T		_s_port_lock;
MUTEX_T		_s_netgr_lock;
RWLOCK_T	_s_si_user_lock;

THREAD_KEY_T	_s_key;

#endif /* _REENTRANT */

void
_init()
{
#ifdef _REENTRANT

	/* Initialize locks */

	MUTEX_INIT(&_s_domain_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&_s_port_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&_s_netgr_lock, USYNC_THREAD, NULL);
	RWLOCK_INIT(&_s_si_user_lock, USYNC_THREAD, NULL);

	/* create keys for per-thread storage */

	THR_KEYCREATE(&_s_key, &_free_s_keytbl);

#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_s_keytbl(void *t)
{
	struct _s_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _s_tsd *)t;

	_free_s_net_info	(key_tbl->net_info_p);
	_free_s_proto_info	(key_tbl->proto_info_p);
	_free_s_serv_info	(key_tbl->serv_info_p);
	_free_s_ndhost_info	(key_tbl->ndhost_info_p);
	_free_s_host_info	(key_tbl->host_info_p);
	_free_s_ifig_info	(key_tbl->ifig_info_p);
	_free_s_token_info	(key_tbl->token_info_p);
	_free_s_netgr_info	(key_tbl->netgr_info_p);
	_free_s_h_errno		(key_tbl->h_errno_p);
	_free_s_ether_s		(key_tbl->ether_s_p);
	_free_s_ether_e		(key_tbl->ether_e_p);
	free			(t);
}

#endif /* _REENTRANT */

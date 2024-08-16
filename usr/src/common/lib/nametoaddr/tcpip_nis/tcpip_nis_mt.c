/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/tcpip_nis/tcpip_nis_mt.c	1.2"
#ident  "$Header: $"

/*
 * tcpip_mt.c
 * This file includes all multi-threading related data and 
 * functions and should be compiled only when _REENTRANT is specified.
 * So this #ifdef will be redundant.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include "tcpip_nis_mt.h"

MUTEX_T	 _ntoa_ypbind_lock;
MUTEX_T	 _ntoa_tcpip_port_lock;
RWLOCK_T _ntoa_tcpip_addr_lock;
THREAD_KEY_T _ntoa_nis_key;

void
_init()
{
	MUTEX_INIT (&_ntoa_tcpip_port_lock, USYNC_THREAD, NULL);
	RWLOCK_INIT(&_ntoa_tcpip_addr_lock, USYNC_THREAD, NULL);
	MUTEX_INIT (&_ntoa_ypbind_lock, USYNC_THREAD, NULL);

	/* create keys for per-thread storage */
	THR_KEYCREATE(&_ntoa_nis_key, &_free_ntoa_nis_keytbl);
}

void
_free_ntoa_nis_keytbl(void *t)
{
	struct _ntoa_nis_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _ntoa_nis_tsd *)t;

	_free_ntoa_nis_hostdata	(key_tbl->hostdata_p);
	_free_ntoa_nis_servdata	(key_tbl->servdata_p);
	_free_ntoa_nis_inet_ntoa(key_tbl->inet_ntoa_p);
	free			(t);
}

#endif /* _REENTRANT */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/netdir/netdir_mt.c	1.2"
#ident	"$Header: $"

/*
 * netdir_mt.c
 * This file includes all multi-threading related data and 
 * functions and should be compiled only when _REENTRANT is specified.
 * So this #ifdef will be redundant.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include <errno.h>
#include <netdir.h>
#include "netdir_mt.h"

RWLOCK_T _netdir_xlist_lock;	/* Should be a rwlock_t when available */

thread_key_t _netdir_key;

void
_netdir_init()
{
	/* initialize synchronization primitives */

	/* The following should be rwlock_init() when available */ 
	RWLOCK_INIT(&_netdir_xlist_lock, USYNC_THREAD, NULL);

	/* create keys for per-thread storage */
	THR_KEYCREATE(&_netdir_key, &_free_netdir_keytbl);
}

void
_free_netdir_keytbl(void *t)
{
	struct _netdir_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _netdir_tsd *)t;

	_free_netdir__nderror	(key_tbl->_nderror_p);
	_free_netdir_errbuf	(key_tbl->errbuf_p);
	free			(t);
}
#endif /* _REENTRANT */

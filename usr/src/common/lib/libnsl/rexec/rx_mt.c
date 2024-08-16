/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rexec/rx_mt.c	1.2"
#ident	"$Header: $"

/*
 * rx_mt.c
 * This file includes all multi-threading related data and 
 * functions and should be compiled only when _REENTRANT is specified.
 * So this #ifdef will be redundant.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include <thread.h>
#include <synch.h>
#include <rx.h>
#include "rx_mt.h"

thread_key_t _rx_key;
MUTEX_T _rx_free_lock;
RWLOCK_T _rx_conn_lock;

void
_rx_init()
{
	if (MULTI_THREADED) {

		/* Initialize locks for free list and connection list. */
		MUTEX_INIT(&_rx_free_lock, USYNC_THREAD, NULL);
		RWLOCK_INIT(&_rx_conn_lock, USYNC_THREAD, NULL);

		/* create key for REXEC's per-thread storage */
		THR_KEYCREATE(&_rx_key, &_free_rx_keytbl);
	}
}

void
_free_rx_keytbl(void *t)
{
	struct _rx_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _rx_tsd *)t;

	_free_rx_errno		(key_tbl->errno_p);
	_free_rx_cserrno	(key_tbl->cserrno_p);
	_free_rx_dflag		(key_tbl->dflag_p);
	free		(t);
}

#endif /* _REENTRANT */

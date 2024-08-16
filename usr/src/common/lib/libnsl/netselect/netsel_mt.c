/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/netselect/netsel_mt.c	1.4"
#ident	"$Header: $"

/*
 * netsel_mt.c
 * This file includes all multi-threading related data and 
 * functions and should be compiled only when _REENTRANT is specified.
 * So this #ifdef will be redundant.
 */

#ifdef _REENTRANT

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netconfig.h>
#include "netcspace.h"
#include "netsel_mt.h"

MUTEX_T _netselect_counter_lock;
RWLOCK_T _netselect_list_lock;

thread_key_t _netselect_key;

void
_netselect_init()
{
	/* initialize synchronization primitives */
	MUTEX_INIT(&_netselect_counter_lock, USYNC_THREAD, NULL);
	RWLOCK_INIT(&_netselect_list_lock, USYNC_THREAD, NULL);

	/* create keys for per-thread storage */
	THR_KEYCREATE(&_netselect_key, &_free_netsel_keytbl);
}

void
_free_netsel_keytbl(void *t)
{
	struct _netsel_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _netsel_tsd *)t;

	_free_netsel_ncerror	(key_tbl->ncerror_p);
	_free_netsel_linenum	(key_tbl->linenum_p);
	_free_netsel_fieldnum	(key_tbl->fieldnum_p);
	_free_netsel_retstr	(key_tbl->retstr_p);
	_free_netsel_savep	(key_tbl->savep_p);
	free			(t);
}

#endif /* _REENTRANT */

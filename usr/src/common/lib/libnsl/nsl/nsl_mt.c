/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/nsl_mt.c	1.1.1.3"
#ident	"$Header: $"

/*
 * nsl_mt.c
 * This file includes all multi-threading related data and 
 * functions and should be compiled only when _REENTRANT is specified.
 * So this #ifdef will be redundant.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include <thread.h>
#include <synch.h>
#include <sys/xti.h>
#include "nsl_mt.h"

thread_key_t _nsl_key;
RWLOCK_T _nsl_lock;

void
_nsl_init()
{
	if (MULTI_THREADED) {

		/* Initialize lock for array of _ti_user pointers. */
		RWLOCK_INIT(&_nsl_lock, USYNC_THREAD, NULL);

		/* create key for TLI's per-thread storage */
		THR_KEYCREATE(&_nsl_key, free);
	}
}

void
_free_nsl_keytbl(void *t)
{
	struct _nsl_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _nsl_tsd *)t;

	_free_nsl_t_errno	(key_tbl->t_errno_p);
	_free_nsl_unk_err_str	(key_tbl->unk_err_str_p);
	free			(t);
}


void
_free_nsl_unk_err_str(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

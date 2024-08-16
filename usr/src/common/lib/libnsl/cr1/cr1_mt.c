/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/cr1/cr1_mt.c	1.1.2.2"
#ident	"$Header: $"

/*
 * cr1_mt.c
 * This file includes all multi-threading related data and 
 * functions and should be compiled only when _REENTRANT is specified.
 * So this #ifdef will be redundant.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include <thread.h>
#include <cr1.h>
#include "cr1_mt.h"

THREAD_KEY_T _cr1_key;

void
_cr1_init()
{
	if (MULTI_THREADED) {

		/* create keys for per-thread storage */
		THR_KEYCREATE(&_cr1_key, &_free_cr1_keytbl);
	}
}

void
_free_cr1_keytbl(void *t)
{
	struct _cr1_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _cr1_tsd *)t;

	_free_cr1_key	(key_tbl->key_p);
	free		(t);
}

#endif /* _REENTRANT */

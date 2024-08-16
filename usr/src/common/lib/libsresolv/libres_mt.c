/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libresolv:common/lib/libsresolv/libres_mt.c	1.3"
#ident	"$Header: $"

/*
 * libres_mt.c
 * This file includes all multi-threading related data and functions
 * for libresolv.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include "libres_mt.h"

THREAD_KEY_T _rs_key;

void
_init()
{
	/* create keys for per-thread storage */
	THR_KEYCREATE(&_rs_key, &_free_rs_keytbl);
}

void
_free_rs_keytbl(void *t)
{
	struct _rs_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _rs_tsd *)t;

	_free_rs_hostinfo	(key_tbl->hostinfo_p);
	_free_rs_nbuf		(key_tbl->nbuf_p);
	_free_rs__res		(key_tbl->_res_p);
	_free_rs_abuf		(key_tbl->abuf_p);
	_free_rs_s		(key_tbl->s_p);
	_free_rs_servinfo	(key_tbl->servinfo_p);
	free			(t);
}

#endif /* _REENTRANT */

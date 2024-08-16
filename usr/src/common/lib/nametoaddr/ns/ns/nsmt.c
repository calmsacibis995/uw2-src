/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/ns/nsmt.c	1.1"
#ident  "$Header: $"

/*
 * libsock_mt.c
 * This file includes the _init() function for initializing libsocket.
 */
#include <stdlib.h>
#include <netdb.h>
#ifdef _REENTRANT
#include "ns_mt.h"

THREAD_KEY_T	_ns_key;

#endif /* _REENTRANT */

void
_init()
{
#ifdef _REENTRANT


	/* create keys for per-thread storage */

	THR_KEYCREATE(&_ns_key, &_free_ns_keytbl);

#endif /* _REENTRANT */
	_nis_init();
}

#ifdef _REENTRANT

void
_free_ns_keytbl(void *t)
{
	struct _ns_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _ns_tsd *)t;

	_free_ns_action		(key_tbl->ns_action_p);
	_free_ns_getent		(key_tbl->ns_getent_p);
	free			(t);
}

#endif /* _REENTRANT */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/novell/novell_mt.c	1.2"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/nametoaddr/novell/novell_mt.c,v 1.2 1994/06/20 16:29:55 mcpowers Exp $"

/*
 * novell_mt.c
 * This file includes all multi-threading related data and 
 * functions and should be compiled only when _REENTRANT is specified.
 * So this #ifdef will be redundant.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include "novell_mt.h"

MUTEX_T _ntoa_novell_firstTime_lock;
MUTEX_T _ntoa_novell_bindresvport_lock;

THREAD_KEY_T _ntoa_novell_key;

void
_init()
{
	/* initialize synchronization primitives */

	MUTEX_INIT (&_ntoa_novell_firstTime_lock, USYNC_THREAD, NULL);
	MUTEX_INIT (&_ntoa_novell_bindresvport_lock, USYNC_THREAD, NULL);

	/* create keys for per-thread storage */
	THR_KEYCREATE(&_ntoa_novell_key, &_free_ntoa_novell_keytbl);
}


/* this free function is called automatically by thr_exit */

void
_free_ntoa_novell_keytbl(void *t)
{
	struct _ntoa_novell_tsd *key_tbl;

	if (t == NULL)
		return;

	key_tbl = (struct _ntoa_novell_tsd *)t;

	_free_ntoa_novell_data (key_tbl->servdata_p);
	_free_ntoa_novell_data (key_tbl->sapdata_p);
	free (t);
}

#endif /* _REENTRANT */

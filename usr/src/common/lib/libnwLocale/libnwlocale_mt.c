/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:libnwlocale_mt.c	1.2"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/libnwlocale_mt.c,v 1.3 1994/09/26 17:20:45 rebekah Exp $"

/*
 * libnwlocale_mt.c
 * This file includes all multi-threading related data and 
 * functions and should be compiled only when _REENTRANT is specified.
 * So this #ifdef will be redundant.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include "ntypes.h"
#include "libnwlocale_mt.h"

MUTEX_T _libnwlocale_lsetloc_lock;
MUTEX_T _libnwlocale__lconvInfo_lock;
MUTEX_T _libnwlocale_loaded_lock;

THREAD_KEY_T _libnwlocale_key;

void
_init()
{
	/* initialize synchronization primitives */

	MUTEX_INIT (&_libnwlocale_lsetloc_lock, USYNC_THREAD, NULL);
	MUTEX_INIT (&_libnwlocale__lconvInfo_lock, USYNC_THREAD, NULL);
	MUTEX_INIT (&_libnwlocale_loaded_lock, USYNC_THREAD, NULL);

	THR_KEYCREATE(&_libnwlocale_key, &_free_libnwlocale_keytbl);
}

/* this free function is called by thr_exit */

void
_free_libnwlocale_keytbl (void *t)
{
	if (t == NULL)
		return;

	free(t);
}

#endif /* _REENTRANT */

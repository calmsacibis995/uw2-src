/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/nwutil_mt.c	1.4"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnutil/nwutil_mt.c,v 1.4 1994/05/09 20:28:15 mark Exp $"

/*
 * nwutil_mt.c
 * This file includes all multi-threading related data and 
 * functions and should be compiled only when _REENTRANT is specified.
 * So this #ifdef will be redundant.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include <mt.h>
#include "nwutil_mt.h"

THREAD_KEY_T domainKey;
THREAD_KEY_T listHeadKey;
MUTEX_T	sap_list_lock;
MUTEX_T	mem_map_lock;
MUTEX_T	head_list_lock;

void
_init()
{
	/* initialize synchronization primitives */

	/* create keys for per-thread storage */
	THR_KEYCREATE(&domainKey, &FreeMsgTSD);
	THR_KEYCREATE(&listHeadKey, &FreeSapTSD);

	/* create locks */
	MUTEX_INIT(&sap_list_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&mem_map_lock, USYNC_THREAD, NULL);
	MUTEX_INIT(&head_list_lock, USYNC_THREAD, NULL);
}


/* these free functions are called automatically by thr_exit */

void
FreeSapTSD(void *t)
{
	if (t == NULL)
		return;

	free (t);
}

void
FreeMsgTSD(void *t)
{
	if (t == NULL)
		return;

	free (t);
}

#endif /* _REENTRANT */

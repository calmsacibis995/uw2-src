/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/yp/yp_mt.c	1.1"
#ident	"$Header: $"

/*
 * yp_mt.c
 * This file includes all multi-threading related data and 
 * functions and should be compiled only when _REENTRANT is specified.
 * So this #ifdef will be redundant.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include <thread.h>
#include <synch.h>
#include <xti.h>
#include "yp_mt.h"

MUTEX_T  _yp_match_lock;
RWLOCK_T _yp_domain_list_lock;

void
_yp_init()
{
	if (MULTI_THREADED) {

		/* Initialize synchronization primitives */
	        MUTEX_INIT(&_yp_match_lock, USYNC_THREAD, NULL);
		RWLOCK_INIT(&_yp_domain_list_lock, USYNC_THREAD, NULL);
	}
}

#endif /* _REENTRANT */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/resolv/resolv_mt.c	1.5"
#ident	"$Header: $"

/*
 * resolv_mt.c
 * This file includes all multi-threading related data and 
 * functions.
 */
#ifdef _REENTRANT

#include <stdlib.h>
#include "resolv_mt.h"

RWLOCK_T _ntoa_rs_addr_lock;

void
_init()
{
	/* initialize synchronization primitives */

	/* The following should be rwlock_init() when available */ 
	RWLOCK_INIT(&_ntoa_rs_addr_lock, USYNC_THREAD, NULL);
}

#endif /* _REENTRANT */

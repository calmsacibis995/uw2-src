/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/nsl/_data.c	1.3.5.2"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/stream.h>
#include "nsl_mt.h"
#include <sys/timod.h>
#include <stdio.h>
#include "_import.h"


/*
 * The array fdp of pointers to _ti_user structures gets allocated
 * the first time the user calls t_open or t_sync.
 * The actual _ti_user structures get allocated each time the user
 * calls t_open or t_sync that results in a new file descriptor.
 */
struct _ti_user **fdp = NULL;

/*
 * This must be here to preserve compatibility
 */
struct _oldti_user _old_ti = { 0, 0, NULL,0,NULL,NULL,NULL,0,0,0,0,0 };

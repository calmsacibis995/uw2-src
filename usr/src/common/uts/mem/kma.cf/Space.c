/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/kma.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>
#include <sys/param.h>
#include <sys/types.h>

/*
 * KMA Parameters
 */

long kma_giveback_time = KMAGBTIME * HZ;
size_t kma_pageout_pool_size = KMA_PAGEOUT_POOL;

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/rawip/rawip.cf/Space.c	1.4"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/bitmasks.h>

/*
 * RAWIP "minor devices" bitmask array.
 * RIP_UNITS (from System file).
 */
int	ripdev_cnt = RIP_UNITS;
uint_t	ripdev_words = BITMASK_NWORDS(RIP_UNITS);
uint_t	ripdev[BITMASK_NWORDS(RIP_UNITS)];

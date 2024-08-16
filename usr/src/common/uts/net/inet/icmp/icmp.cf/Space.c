/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/icmp/icmp.cf/Space.c	1.3"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/bitmasks.h>

/*
 * ICMP "minor devices" bitmask array.
 * ICMP_UNITS (from System file).
 */
int	icmpdev_cnt = ICMP_UNITS;
uint_t	icmpdev_words = BITMASK_NWORDS(ICMP_UNITS);
uint_t	icmpdev[BITMASK_NWORDS(ICMP_UNITS)];
/*
 * Switch to enable answering of subnet mask requests
 * ICMPMASKREQ is a tunable.
 */
boolean_t	icmp_answermask = ICMPMASKREQ;

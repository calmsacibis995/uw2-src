/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/inet/udp/udp.cf/Space.c	1.5"
#ident	"$Header: $"

#include <config.h>
#include <sys/types.h>
#include <sys/bitmasks.h>

/*
 * UDP "minor devices" bitmask array.
 * UDP_UNITS (from System file).
 */
int	udpdev_cnt = UDP_UNITS;
uint_t	udpdev_words = BITMASK_NWORDS(UDP_UNITS);
uint_t	udpdev[BITMASK_NWORDS(UDP_UNITS)];
/*
 * Default time-to-live for UDP packets.
 * UDPTTL is a tunable.
 */
unsigned char	udpttl = UDPTTL;
/*
 * Switch to enable checksumming.
 * UDPCKSUM is a tunable.
 */
int	udpcksum = UDPCKSUM;

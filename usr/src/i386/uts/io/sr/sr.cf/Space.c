/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1993 UNIVEL */

#ident	"@(#)kern-i386:io/sr/sr.cf/Space.c	1.1"
#ident	"$Header: $"

#include	<sys/types.h>
#include	<sys/stream.h>
#include	<sys/sr.h>


extern int	sr_insert_age();		/* update table with new source
						 * route information only if
						 * if the entry has aged sr_age
						 * ticks
						 */
extern int	sr_insert_latest();		/* update table with latest
						 * source route information
						 */

int	(*sr_insert)() = sr_insert_latest;	/* use the latest source route
						 * information
						 */

#define		ARPT_KILLC	(20 * 60)	/* from ARP - kill completed
						 * entry in 20 mins.
						 */

int	sr_age = ARPT_KILLC;			/* used only if sr_insert is
						 * set to sr_insert_age
						 */

int	sr_broadcast = SINGLE_ROUTE_BCAST;	/* change to ALL_ROUTE_BCAST if
						 * needed
						 */


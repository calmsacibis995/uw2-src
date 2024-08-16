/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1993 UNIVEL */

#ident	"@(#)kern-i386at:io/dlpi_cpq/cet/sr/nflxsr.cf/Space.c	1.4"
#ident	"$Header: $"

#ifndef	UW1_1
#define	ESMP
#endif

#include	<sys/types.h>
#include	<sys/stream.h>
#include	<sys/nflxsr.h>
#include	<config.h>


extern int	nflxsr_insert_age();		/* update table with new source
						 * route information 
						 * periodically 
						 */
extern int	nflxsr_insert_latest();		/* update table with latest
						 * source route information
						 */

int	(*nflxsr_insert)() = nflxsr_insert_age; /* use the age source route
						 * information
						 */

#define		ARPT_KILLC	(20 * 60)	/* from ARP - kill completed
						 * entry in 20 mins.
						 */

int	nflxsr_age = NFLXSR_AGE;		/* used only if nflxsr_insert is
						 * set to nflxsr_insert_age
						 * in minutes
						 */

int	nflxsr_broadcast = SINGLE_ROUTE_BCAST;	/* change to ALL_ROUTE_BCAST if
						 * needed
						 */


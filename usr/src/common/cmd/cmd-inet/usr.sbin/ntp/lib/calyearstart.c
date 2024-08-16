/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/calyearstart.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
 * calyearstart - determine the NTP time at midnight of January 1 in
 *		  the year of the given date.
 */
#include <sys/types.h>

#include "ntp_calendar.h"

/*
 * calyeartab - year start offsets from the beginning of a cycle
 */
u_long calyeartab[YEARSPERCYCLE] = {
	(SECSPERLEAPYEAR-JANFEBLEAP),
	(SECSPERLEAPYEAR-JANFEBLEAP) + SECSPERYEAR,
	(SECSPERLEAPYEAR-JANFEBLEAP) + 2*SECSPERYEAR,
	(SECSPERLEAPYEAR-JANFEBLEAP) + 3*SECSPERYEAR
};

u_long
calyearstart(dateinyear)
	register u_long dateinyear;
{
	register u_long cyclestart;
	register u_long nextyear, lastyear;
	register int i;

	/*
	 * Find the start of the cycle this is in.
	 */
	if (dateinyear >= MAR1988)
		cyclestart = MAR1988;
	else
		cyclestart = MAR1900;
	while ((cyclestart + SECSPERCYCLE) <= dateinyear)
		cyclestart += SECSPERCYCLE;
	
	/*
	 * If we're in the first year of the cycle, January 1 is
	 * two months back from the cyclestart and the year is
	 * a leap year.
	 */
	lastyear = cyclestart + calyeartab[0];
	if (dateinyear < lastyear)
		return (cyclestart - JANFEBLEAP);

	/*
	 * Look for an intermediate year
	 */
	for (i = 1; i < YEARSPERCYCLE; i++) {
		nextyear = cyclestart + calyeartab[i];
		if (dateinyear < nextyear)
			return lastyear;
		lastyear = nextyear;
	}

	/*
	 * Not found, must be in last two months of cycle
	 */
	return nextyear;
}

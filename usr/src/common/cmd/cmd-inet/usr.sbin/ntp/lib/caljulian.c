/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/caljulian.c	1.2"
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
 * caljulian - determine the Julian date from an NTP time.
 */
#include <sys/types.h>

#include "ntp_calendar.h"

/*
 * calmonthtab - month start offsets from the beginning of a cycle.
 */
static u_short calmonthtab[12] = {
	0,						/* March */
	MAR,						/* April */
	(MAR+APR),					/* May */
	(MAR+APR+MAY),					/* June */
	(MAR+APR+MAY+JUN),				/* July */
	(MAR+APR+MAY+JUN+JUL),				/* August */
	(MAR+APR+MAY+JUN+JUL+AUG),			/* September */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP),			/* October */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT),		/* November */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV),		/* December */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC),	/* January */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC+JAN),	/* February */
};

/*
 * caldaytab - calendar year start day offsets
 */
static u_short caldaytab[YEARSPERCYCLE] = {
	(DAYSPERYEAR - (JAN + FEB)),
	((DAYSPERYEAR * 2) - (JAN + FEB)),
	((DAYSPERYEAR * 3) - (JAN + FEB)),
	((DAYSPERYEAR * 4) - (JAN + FEB)),
};

void
caljulian(ntptime, jt)
	u_long ntptime;
	register struct calendar *jt;
{
	register int i;
	register u_long nt;
	register u_short snt;
	register int cyear;

	/*
	 * Find the start of the cycle this is in.
	 */
	nt = ntptime;
	if (nt >= MAR1988) {
		cyear = CYCLE22;
		nt -= MAR1988;
	} else {
		cyear = 0;
		nt -= MAR1900;
	}
	while (nt >= SECSPERCYCLE) {
		nt -= SECSPERCYCLE;
		cyear++;
	}
	
	/*
	 * Seconds, minutes and hours are too hard to do without
	 * divides, so we don't.
	 */
	jt->second = nt % SECSPERMIN;
	nt /= SECSPERMIN;		/* nt in minutes */
	jt->minute = nt % MINSPERHR;
	snt = nt / MINSPERHR;		/* snt in hours */
	jt->hour = snt % HRSPERDAY;
	snt /= HRSPERDAY;		/* nt in days */

	/*
	 * snt is now the number of days into the cycle, from 0 to 1460.
	 */
	cyear <<= 2;
	if (snt < caldaytab[0]) {
		jt->yearday = snt + JAN + FEBLEAP + 1;	/* first year is leap */
	} else {
		for (i = 1; i < YEARSPERCYCLE; i++)
			if (snt < caldaytab[i])
				break;
		jt->yearday = snt - caldaytab[i-1] + 1;
		cyear += i;
	}
	jt->year = cyear + 1900;

	/*
	 * One last task, to compute the month and day.  Normalize snt to
	 * a day within a cycle year.
	 */
	while (snt >= DAYSPERYEAR)
		snt -= DAYSPERYEAR;
	for (i = 0; i < 11; i++)
		if (snt < calmonthtab[i+1])
			break;
	
	if (i > 9)
		jt->month = i - 9;	/* January or February */
	else
		jt->month = i + 3;	/* March through December */
	jt->monthday = snt - calmonthtab[i] + 1;
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/caltontp.c	1.2"
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
 * caltontp - convert a julian date to an NTP time
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

u_long
caltontp(jt)
	register struct calendar *jt;
{
	register int cyear;
	register int resyear;
	register u_long nt;
	register int yearday;

	/*
	 * Find the start of the cycle this is in.
	 */
	cyear = (int) (jt->year - 1900) >> 2;
	resyear = (jt->year - 1900) - (cyear << 2);
	yearday = 0;
	if (resyear == 0) {
		if (jt->yearday == 0) {
			if (jt->month == 1 || jt->month == 2)
				cyear--;
				resyear = 3;
		} else {
			if ((int) jt->yearday <= (JAN+FEBLEAP)) {
				cyear--;
				resyear = 3;
				yearday = calmonthtab[10] + jt->yearday;
			} else {
				yearday = jt->yearday - (JAN+FEBLEAP);
			}
		}
	} else {
		if (jt->yearday == 0) {
			if (jt->month == 1 || jt->month == 2)
				resyear--;
		} else {
			if ((int) jt->yearday <= JAN+FEB) {
				resyear--;
				yearday = calmonthtab[10] + jt->yearday;
			} else {
				yearday = jt->yearday - (JAN+FEB);
			}
		}
	}

	if (yearday == 0) {
		if (jt->month >= 3) {
			yearday = calmonthtab[jt->month - 3] + jt->monthday;
		} else {
			yearday = calmonthtab[jt->month + 9] + jt->monthday;
		}
	}

	nt = TIMESDPERC((u_long)cyear);
	while (resyear-- > 0)
		nt += DAYSPERYEAR;
	nt += (u_long) (yearday - 1);

	nt = TIMES24(nt) + (u_long)jt->hour;
	nt = TIMES60(nt) + (u_long)jt->minute;
	nt = TIMES60(nt) + (u_long)jt->second;

	return nt + MAR1900;
}

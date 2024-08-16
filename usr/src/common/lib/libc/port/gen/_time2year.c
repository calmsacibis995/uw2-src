/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_time2year.c	1.1"

#include <time.h>
#include "timem.h"

void
#ifdef __STDC__
_time2year(struct year_info *yp, long secs)
#else
_time2year(yp, secs)struct year_info *yp; long secs;
#endif
{
	long days, years;

	days = _ldivrem(&secs, SEC_DAY, 0L);
	yp->day = days;
	yp->sec = secs;
	years = _ldivrem(&days, DAY_MINYEAR, EPOCH_YEAR * DAY_MINYEAR);
	if (years > 0)
	{
		days -= _nlday(years);	/* correct for leap days */
		while (days < 0)	/* years are (still) too high */
		{
			years--;
			days += DAY_MINYEAR;
			if (ISLEAPYEAR(years))
				days++;
		}
	}
	yp->year = years;
	yp->yday = days;
}

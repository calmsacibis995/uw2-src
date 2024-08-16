/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_time2tm.c	1.1"

#include "synonyms.h"
#include <limits.h>
#include <time.h>
#include "timem.h"

void
#ifdef __STDC__
_time2tm(struct tm *ptm, long secs)
#else
_time2tm(ptm, secs)struct tm *ptm; long secs;
#endif
{
	struct year_info ybuf;
	int days, n;

	ptm->tm_isdst = 0;
	_time2year(&ybuf, secs);
	if (ybuf.year < INT_MIN + BASE_YEAR)
		ptm->tm_year = INT_MIN;
#if INT_MAX != LONG_MAX
	else if (ybuf.year > INT_MAX - BASE_YEAR)
		ptm->tm_year = INT_MAX;
#endif
	else
		ptm->tm_year = ybuf.year - BASE_YEAR;
	(void)_ldivrem(&ybuf.day, DAY_WEEK, EPOCH_WDAY);
	ptm->tm_wday = ybuf.day;
	ptm->tm_yday = ybuf.yday;
	/*
	* Set month and day of month.
	*/
	if (ybuf.yday < DAY_JAN + DAY_MINFEB) /* unaffected by leap years */
	{
		if (ybuf.yday < DAY_JAN)
			ptm->tm_mon = 0;
		else
		{
			ptm->tm_mon = 1;
			ybuf.yday -= DAY_JAN;
		}
	}
	else /* leap year matters */
	{
		ybuf.yday -= DAY_JAN + DAY_MINFEB;
		if (ISLEAPYEAR(ybuf.year) && --ybuf.yday < 0) /* was Feb. 29 */
		{
			ptm->tm_mon = 1;
			ybuf.yday = DAY_MINFEB;
		}
		else /* check the rest of the months */
		{
			int i, m;

			for (i = 2; (m = _tm_day_mon[i]) <= ybuf.yday; i++)
				ybuf.yday -= m;
			ptm->tm_mon = i;
		}
	}
	ptm->tm_mday = 1 + ybuf.yday;
	/*
	* Finally, set hours, minutes, and seconds.
	*/
	ptm->tm_hour = n = ybuf.sec / SEC_HOUR;
	ybuf.sec -= n * SEC_HOUR;
	ptm->tm_min = n = ybuf.sec / SEC_MIN;
	ptm->tm_sec = ybuf.sec - n * SEC_MIN;
}

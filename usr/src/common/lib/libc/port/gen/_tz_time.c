/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_tz_time.c	1.2"

#include "synonyms.h"
#include <time.h>
#include "timem.h"

struct date_info
{
	int	ly;	/* nonzero => leap year */
	int	wd;	/* wday for January 1st */
};

static const char *
#ifdef __STDC__
posixdate(const char *p, long *s, const struct date_info *dp)
#else
posixdate(p, s, dp)const char *p; long *s; const struct date_info *dp;
#endif
{
	long sec;
	int day;

	if (*p == 'J') /* J<n> [1,DAY_MINYEAR]: day ignoring 29 Feb */
	{
		if ((p = _tz_int(p + 1, &day)) == 0
			|| --day < 0 || day >= DAY_MINYEAR)
		{
			return 0;
		}
		if (day >= DAY_JAN + DAY_MINFEB && dp->ly != 0)
			day++;
	}
	else if (*p != 'M') /* <n> [0,DAY_MINYEAR]: day including 29 Feb */
	{
		if ((p = _tz_int(p, &day)) == 0
			|| day < 0 || day > DAY_MINYEAR)
		{
			return 0;
		}
	}
	else /* M<m>.<w>.<d> [1,MON_YEAR].[1,5].[0,DAY_WEEK-1] */
	{
		int mon, week, wday, wd;

		if ((p = _tz_int(p + 1, &mon)) == 0 || *p != '.'
			|| --mon < 0 || mon >= MON_YEAR
			|| (p = _tz_int(p + 1, &week)) == 0 || *p != '.'
			|| week < 1 || week > 5
			|| (p = _tz_int(p + 1, &wday)) == 0
			|| wday < 0 || wday >= DAY_WEEK)
		{
			return 0;
		}
		/*
		* Set "day" to weekday "wday" of week "week" of month "mon",
		* where if "week" is too high, the last "wday" is used.
		*/
		day = _tm_cum_day_mon[mon];
		mon = _tm_day_mon[mon];
		if (dp->ly != 0)
		{
			if (day >= DAY_JAN + DAY_MINFEB)
				day++;
			else if (day >= DAY_JAN)
				mon++;
		}
		wd = (dp->wd + day) % DAY_WEEK;
		while (wd != wday)
		{
			day++;
			mon--;
			if (++wd >= DAY_WEEK)
				wd = 0;
		}
		while (--week != 0 && mon > DAY_WEEK)
		{
			day += DAY_WEEK;
			mon -= DAY_WEEK;
		}
	}
	sec = day * SEC_DAY;
	if (*p != '/')
		sec += 2 * SEC_HOUR;
	else
	{
		long rem;

		if ((p = _tz_hms(p + 1, &rem)) == 0)
			return 0;
		sec += rem;
	}
	*s = sec;
	return p;
}

static const char *
#ifdef __STDC__
compatdate(const char *p, long *s)
#else
compatdate(p, s)const char *p; long *s;
#endif
{
	long sec;
	int day;

	if ((p = _tz_int(p, &day)) == 0)
		return 0;
	if (--day < 0 || day > DAY_MINYEAR)
		return 0;
	sec = day * SEC_DAY;
	if (*p == '/')
	{
		long rem;

		if ((p = _tz_hms(p + 1, &rem)) == 0)
			return 0;
		sec += rem;
	}
	*s = sec;
	return p;
}

int
#ifdef __STDC__
_tz_time(struct tz_info *ptz, long secs)
#else
_tz_time(ptz, secs)struct tz_info *ptz; long secs;
#endif
{
	struct year_info ybuf;
	struct date_info dbuf;
	long start, after;
	const char *p;

	_time2year(&ybuf, secs);
	secs = ybuf.yday * SEC_DAY + ybuf.sec;
	/*
	* Determine the start and end points (in seconds)
	* since the start of the current year.
	*/
	if ((p = ptz->etc) == 0)	/* use U.S.A. rules */
	{
		static const char oldusa[] = "M4.5.0,M10.5.0";
		static const short usayears[] = {1987, 1976, 1975, 1974};
		static const char *const usarules[] =
		{
			"M4.1.0,M10.5.0", oldusa, "53,298", "5,327"
		};
		/*
		* Key to the above:
		* - originally DST was from the last Sunday in April
		*	to the last Sunday in October.
		* - for 1974 only, DST was from the first Sunday in January
		*	(the 6th) to the last Sunday in November (the 24th).
		*	[The ending day is apparently in dispute.  The file
		*	use for _tz_file() says the last Sunday in October!]
		* - for 1975 only, DST was from the last Sunday in February
		*	(the 23rd) to the last Sunday in October (the 26th).
		* - for 1976-1986, the original rule again was applied.
		* - from 1987 on, DST runs from the first Sunday in April
		*	to the last Sunday in October.
		*/
		int i = 0;

		while (ybuf.year < usayears[i])
		{
			if (++i == sizeof(usayears) / sizeof(usayears[0]))
			{
				p = oldusa;
				goto posix;
			}
		}
		p = usarules[i];
		goto posix;
	}
	else if (*p++ == ';')	/* old-style start and end */
	{
		if ((p = compatdate(p, &start)) == 0 || *p != ','
			|| (p = compatdate(p + 1, &after)) == 0)
		{
			return 0;
		}
	}
	else /* fancier POSIX.1 start and end */
	{
	posix:;
		/*
		* Compute once the weekday for January 1st and whether
		* it's a leap year, even though neither may be needed.
		*/
		ybuf.day -= ybuf.yday;
		(void)_ldivrem(&ybuf.day, DAY_WEEK, EPOCH_WDAY);
		dbuf.wd = ybuf.day;
		dbuf.ly = 0;
		if (ISLEAPYEAR(ybuf.year))
			dbuf.ly = 1;
		if ((p = posixdate(p, &start, &dbuf)) == 0 || *p != ','
			|| (p = posixdate(p + 1, &after, &dbuf)) == 0)
		{
			return 0;
		}
	}
	start += ptz->off[0];
	after += ptz->off[1];
	/*
	* Return whether the time in hand is within the alternate timezone.
	*/
	if (start < after)
	{
		if (start <= secs && secs < after)
			return 1;
	}
	else
	{
		if (secs < after || start <= secs)
			return 1;
	}
	return 0;
}

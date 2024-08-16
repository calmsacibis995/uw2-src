/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_norm_tm.c	1.1"

#include <time.h>
#include <limits.h>
#include "timem.h"

static int
#ifdef __STDC__
muladd(long *sum, long value, long scale) /* *sum += value*scale; scale>0 */
#else
muladd(sum, value, scale)long *sum, value, scale;
#endif
{
	if (value < -1)
	{
		if (LONG_MIN / value < scale)
			return -1;
	}
	else if (value > 1)
	{
		if (LONG_MAX / value < scale)
			return -1;
	}
	value *= scale;
	if (*sum > 0)
	{
		if (LONG_MAX - *sum < value)
			return -1;
	}
	else
	{
		if (LONG_MIN - *sum > value)
			return -1;
	}
	*sum += value;
	return 0;
}

long
#ifdef __STDC__
_norm_tm(struct tm *ptm) /* correct tm_*'s, return seconds relative to Epoch */
#else
_norm_tm(ptm)struct tm *ptm;
#endif
{
	int i, nld, epyr, flag;
	long val;

	flag = 0;
	/*
	* First, normalize from the finest granulatity up the line,
	* each step spilling into (or borrowing from) the next bucket.
	* Temporarily adjust tm_mday's and tm_year's origins to 0.
	* In this pass, pretend as if all months have maximum length.
	*/
	i = _idivrem(&ptm->tm_sec, SEC_MIN, 0);
	i = _idivrem(&ptm->tm_min, MIN_HOUR, i);
	i = _idivrem(&ptm->tm_hour, HOUR_DAY, i);
	i = _idivrem(&ptm->tm_mday, DAY_MAXMON, i - 1);
	i = _idivrem(&ptm->tm_mon, MON_YEAR, i);
	i += BASE_YEAR;
	if (ptm->tm_year > 0)
	{
		if (INT_MAX - ptm->tm_year < i)
		{
			flag = -1;
			ptm->tm_year = INT_MAX;
			epyr = INT_MAX - EPOCH_YEAR;
			goto skip;
		}
	}
	else
	{
		if (INT_MIN + EPOCH_YEAR - ptm->tm_year > i)
		{
			flag = -1;
			ptm->tm_year = INT_MIN + EPOCH_YEAR;
			epyr = INT_MIN;
			goto skip;
		}
	}
	if ((ptm->tm_year += i) <= 0)
		flag = -1;	/* cannot handle B.C. */
	epyr = ptm->tm_year - EPOCH_YEAR;
skip:;
	/*
	* Note in flag whether it's a leap year.
	*/
	if (flag == 0 && ISLEAPYEAR(ptm->tm_year))
		flag = 1;
	/*
	* Go back and fix tm_mday.  The only remaining special case is
	* February 29 which is not a spill into March for leap years.
	*/
	if ((i = _tm_day_mon[ptm->tm_mon]) <= ptm->tm_mday)
	{
		if (flag <= 0 || i != DAY_MINFEB
			|| ptm->tm_mday != DAY_MINFEB)
		{
			ptm->tm_mday -= i;
			if (++ptm->tm_mon == MON_YEAR)
			{
				ptm->tm_mon = 0;
				if (ptm->tm_year == INT_MAX)
					flag = -1;
				else
					ptm->tm_year++;
				epyr++;
			}
		}
	}
	/*
	* Determine tm_yday, nld, and tm_wday.
	*/
	if ((ptm->tm_yday = _tm_cum_day_mon[ptm->tm_mon] + ptm->tm_mday)
		>= DAY_JAN + DAY_MINFEB)
	{
		if (ptm->tm_mon > 1 && flag > 0)
			ptm->tm_yday++;
	}
	if (ptm->tm_year <= 0)
		nld = 0;
	else
		nld = _nlday(ptm->tm_year);
	val = nld + ptm->tm_yday;
	(void)muladd(&val, (long)epyr, DAY_MINYEAR);
	(void)_ldivrem(&val, DAY_WEEK, EPOCH_WDAY);
	ptm->tm_wday = val;
	if (flag >= 0)	/* still viable */
	{
		/*
		* Try to compute the seconds since the Epoch.
		* It can only overflow now because of a large tm_year.
		*/
		val = ptm->tm_sec + SEC_MIN * ptm->tm_min
			+ SEC_HOUR * ptm->tm_hour + SEC_DAY * ptm->tm_yday;
		if (muladd(&val, (long)epyr, SEC_MINYEAR) != 0
			|| muladd(&val, (long)nld, SEC_DAY) != 0)
		{
			flag = -1;
		}
	}
	/*
	* Convert tm_mday and tm_year back to their nonzero origins.
	*/
	ptm->tm_mday++;
	ptm->tm_year -= BASE_YEAR;
	if (flag < 0)
		return LONG_MIN;
	return val;
}

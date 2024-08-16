/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/mktime.c	1.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <limits.h>
#include <time.h>
#include "timem.h"

static long
#ifdef __STDC__
add(long t, long off)
#else
add(t, off)long t, off;
#endif
{
	if (off > 0)
	{
		if (t > LONG_MAX - off)
			return LONG_MAX;
	}
	else if (off < 0)
	{
		if (t < LONG_MIN - off)
			return LONG_MIN;
	}
	return t + off;
}

time_t
#ifdef __STDC__
mktime(struct tm *ptm)
#else
mktime(ptm)struct tm *ptm;
#endif
{
	struct tz_info tzbuf;
	int isdst, wasdst;
	long t;

#ifdef CALL_TZSET
	tzset();
#endif
	if ((t = _norm_tm(ptm)) == LONG_MIN)	/* overflowed */
		return -1;
	_tz_info(&tzbuf);
	/*
	* Now the "fun" part...unbiasing the local time back to UTC.
	* Basically, assume that the incoming tm_isdst is correct
	* (except that for "no information" we use the main timezone),
	* and then refill the tm structure if the guess appears bad.
	*/
	wasdst = ptm->tm_isdst;
	if (tzbuf.str[0][0] != '\0' && tzbuf.str[1][0] != '\0')
	{
		/*
		* Have two timezones and the offsets are fixed.
		* Normalize tm_isdst and use it as the initial
		* guess at the true timezone.
		*/
		if (wasdst < 0)
			ptm->tm_isdst = 0;
		else if (wasdst > 1)
			ptm->tm_isdst = 1;
		t = add(t, tzbuf.off[ptm->tm_isdst]);
		isdst = _tz_time(&tzbuf, t);
	}
	else
	{
		if (tzbuf.str[0][0] != '\0') /* only a main timezone */
		{
			ptm->tm_isdst = 0;
			isdst = 0;
		}
		else
		{
			/*
			* The offset(s) for the timezone(s) can depend
			* on the value for t.  Have _tz_file() do the
			* guessing for us based on the incoming value
			* for tm_isdst (which we also normalize).
			*/
			if (wasdst > 0)
			{
				ptm->tm_isdst = 1;
				isdst = 2;
			}
			else if ((isdst = wasdst) < 0)
				ptm->tm_isdst = 0;
			else
				isdst = 1;
			isdst = _tz_file(&tzbuf, t, isdst);
			/*
			* Now that we have the offset(s), make sure
			* that tm_isdst is still valid.
			*/
			if (ptm->tm_isdst != 0 && tzbuf.str[1][0] == '\0')
				ptm->tm_isdst = 0;
		}
		t = add(t, tzbuf.off[ptm->tm_isdst]);
	}
	/*
	* Now we're pretty sure that isdst denotes the correct setting
	* for tm_isdst.  If the guess was wrong above, do the equivalent
	* of a localtime(t) based on isdst.  Moreover, if mktime was told
	* to guess (wasdst < 0), rebias back to UTC for the alternate.
	*/
	if (isdst != ptm->tm_isdst) /* ugh...reset entire structure */
	{
		if (wasdst < 0)	/* mktime guessed wrong */
			t = add(t, tzbuf.off[1] - tzbuf.off[0]);
		_time2tm(ptm, add(t, -tzbuf.off[isdst]));
		ptm->tm_isdst = isdst;
	}
	/*CONSTANTCONDITION*/
	if (sizeof(time_t) < sizeof(long))
	{
		if (t < INT_MIN || INT_MAX < t)	/* presumes time_t is int */
			return -1;
	}
	return t;
}

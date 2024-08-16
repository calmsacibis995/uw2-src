/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/ctime_r.c	1.1"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <limits.h>
#include <stdio.h>
#include <time.h>
#include "timem.h"

#ifdef __STDC__
	#pragma weak asctime_r = _asctime_r
	#pragma weak ctime_r = _ctime_r
	#pragma weak gmtime_r = _gmtime_r
	#pragma weak localtime_r = _localtime_r
#endif

char *
#ifdef __STDC__
asctime_r(const struct tm *ptm, char *buf)
#else
asctime_r(ptm, buf)const struct tm *ptm; char *buf;
#endif
{
	snprintf(buf, (size_t)26, "%s %s %2d %.2d:%.2d:%.2d %d\n",
		_str_abday[ptm->tm_wday], _str_abmon[ptm->tm_mon],
		ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec,
		ptm->tm_year + BASE_YEAR);
	return buf;
}

char *
#ifdef __STDC__
ctime_r(const time_t *tp, char *buf)
#else
ctime_r(tp, buf)const time_t *tp; char *buf;
#endif
{
	struct tm tmbuf;

	asctime_r(localtime_r(tp, &tmbuf), buf);
	return buf;
}

double
#ifdef __STDC__
difftime(time_t t1, time_t t0)
#else
difftime(t1, t0)time_t t1, t0;
#endif
{
	return t1 - t0;
}

struct tm *
#ifdef __STDC__
gmtime_r(const time_t *tp, struct tm *ptm)
#else
gmtime_r(tp, ptm)const time_t *tp; struct tm *ptm;
#endif
{
	_time2tm(ptm, (long)*tp);
	return ptm;
}

struct tm *
#ifdef __STDC__
localtime_r(const time_t *tp, struct tm *ptm)
#else
localtime_r(tp, ptm)const time_t *tp; struct tm *ptm;
#endif
{
	struct tz_info tzbuf;
	int dst;
	long t, off;

#ifdef CALL_TZSET
	tzset();
#endif
	t = *tp;
	_tz_info(&tzbuf);
	if (tzbuf.str[0][0] == '\0')
		dst = _tz_file(&tzbuf, t, 0);
	else if (tzbuf.str[1][0] != '\0')
		dst = _tz_time(&tzbuf, t);
	else
		dst = 0;
	/*
	* Adjust the seconds to reflect the active timezone's offset.
	*/
	if ((off = tzbuf.off[dst]) > 0)
	{
		if (t < LONG_MIN + off)
			off = t - LONG_MIN;
	}
	else if (off < 0)
	{
		if (t > LONG_MAX + off)
			off = t - LONG_MAX;
	}
	_time2tm(ptm, t - off);
	ptm->tm_isdst = dst;
	return ptm;
}

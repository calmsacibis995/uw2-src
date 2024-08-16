/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/strptime.c	1.3"

#include "synonyms.h"
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <time.h>
#include "timem.h"

#ifdef __STDC__
	#pragma weak strptime = _strptime
#endif

static const char *
#ifdef __STDC__
samestr(register const char *src, register const char *str)
#else
samestr(src, str)register const char *src, *str;
#endif
{
	register int ch1, ch2;

	/*
	* Compare "src" sequence against string "str", ignoring case.
	* "str" is assumed to have at least one character.
	*/
	do
	{
		if (isupper(ch1 = *src))
			ch1 = _tolower(ch1);
		if (isupper(ch2 = *str))
			ch2 = _tolower(ch2);
		if (ch1 != ch2)
			return 0;
	} while (++src, *++str != '\0');
	return src;
}

static const char *
#ifdef __STDC__
matchstr(int *ip, const char *src, int len, const char **list)
#else
matchstr(ip, src, len, list)int *ip; const char *src; int len; const char **list;
#endif
{
	const char *p;
	int i;

	while (isspace(*src))	/* skip any white space */
		src++;
	/*
	* Look for match of "src" against the members of "list".
	*/
	i = 0;
	while ((p = samestr(src, *list++)) == 0)
	{
		if (++i >= len)
			return 0;
	}
	*ip = i;
	return p;
}

static const char *
#ifdef __STDC__
matchnum(int *ip, const char *src)
#else
matchnum(ip, src)int *ip; const char *src;
#endif
{
	int i;

	while (isspace(*src))	/* skip any white space */
		src++;
	/*
	* Match any (unsigned) base 10 number-like sequence.
	*/
	if (!isdigit(*src))
		return 0;
	i = *src - '0';
	while (isdigit(*++src))	/* ignores overflows */
	{
		i *= 10;
		i += *src - '0';
	}
	*ip = i;
	return src;
}

struct recur	/* items common to all invocations of parse() */
{
	struct tm	t;
	struct lc_time	*plt;
	struct era_info	*ep;
	int		year;
	int		week;
	int		code;
};

static const char *
#ifdef __STDC__
eraname(const char *src, struct recur *rp)
#else
eraname(src, rp)const char *src; struct recur *rp;
#endif
{
	struct era_info *ep = rp->ep;
	const char *p;

	while (isspace(*src))	/* skip any white space */
		src++;
	do
	{
		if (ep == 0 && (ep = rp->plt->extra->head) == 0)
			break;
		if ((p = samestr(src, ep->name)) != 0)
		{
			rp->ep = ep;
			return p;
		}
	} while ((ep = ep->next) != rp->ep);
	return 0;
}

#ifdef __STDC__
static const char *parse(const char *, const char *, struct recur *);
#else
static const char *parse();
#endif

static const char *
#ifdef __STDC__
erafmt(const char *src, struct recur *rp)
#else
erafmt(src, rp)const char *src; struct recur *rp;
#endif
{
	struct recur cpy;
	struct era_info *ep = rp->ep;
	const char *p;

	while (isspace(*src))	/* skip any white space */
		src++;
	do
	{
		if (ep == 0 && (ep = rp->plt->extra->head) == 0)
			break;
		cpy = *rp;
		if ((p = parse(src, ep->fmt, &cpy)) != 0)
		{
			*rp = cpy;
			return p;
		}
	} while ((ep = ep->next) != rp->ep);
	return 0;
}

static const char *
#ifdef __STDC__
parse(const char *src, const char *fmt, struct recur *rp)
#else
parse(src, fmt, rp)const char *src, *fmt;struct recur *rp;
#endif
{
	struct lc_time *plt = rp->plt;
	struct lc_time_era *era = rp->plt->extra;
	const char *str;
	int flag, num;

	num = -1;
	for (flag = '\0';; fmt++)
	{
	numloop:;
		switch (*fmt)
		{
		recur:;
			if ((src = parse(src, str, rp)) == 0)
				return 0;
			break;
		number:;
			if (flag == 'O')
			{
				if ((src = matchstr(&num, src, plt->lastnum,
					era->altnum)) == 0)
				{
					return 0;
				}
			}
			else if ((src = matchnum(&num, src)) == 0)
				return 0;
			goto numloop;
		case '\0':
			return src;
		case 'n':	/* white space */
		case 't':
			if (flag == '\0')
				goto regchar;
			flag = '\0';
			/*FALLTHROUGH*/
		case ' ':	/* white space */
		case '\t':
		case '\n':
		case '\r':
		case '\f':
		case '\v':
			while (isspace(*src))
				src++;
			continue;
		default:
		regchar:;
			if (*src++ != *fmt)
				return 0;
			continue;
		case '%':
			if (flag == '\0')
				flag = '%';
			else if (*src++ != '%')
				return 0;
			else
				flag = '\0';
			continue;
		case 'a':	/* weekday name */
		case 'A':
			if (flag == '\0')
				goto regchar;
			/*
			* Must check full length names first.
			*/
			if ((str = matchstr(&num, src, DAY_WEEK, plt->day)) == 0
				&& (str = matchstr(&num, src, DAY_WEEK,
				plt->abday)) == 0)
			{
				return 0;
			}
			rp->t.tm_wday = num;
			src = str;
			break;
		case 'h':	/* month name */
		case 'b':
		case 'B':
			if (flag == '\0')
				goto regchar;
			/*
			* Must check full length names first.
			*/
			if ((str = matchstr(&num, src, MON_YEAR, plt->mon)) == 0
				&& (str = matchstr(&num, src, MON_YEAR,
				plt->abmon)) == 0)
			{
				return 0;
			}
			rp->t.tm_mon = num;
			src = str;
			break;
		case 'c':	/* default format string */
			if (flag == '\0')
				goto regchar;
			if (flag == 'E')
				str = era->erabothfmt;
			else
				str = plt->bothfmt;
			goto recur;
		case 'C':	/* century number or era name */
			if (flag == '\0')
				goto regchar;
			if (flag == 'E')
			{
				if ((src = eraname(src, rp)) == 0)
					return 0;
				break;
			}
			if (num < 0)	/* [0,INT_MAX/100] */
				goto number;
			else if (num > INT_MAX / 100)
				return 0;
			if (rp->t.tm_year < 0)
				rp->t.tm_year = 0;
			else
				rp->t.tm_year %= 100;
			rp->t.tm_year += num * 100;
			break;
		case 'D':
			if (flag == '\0')
				goto regchar;
			str = "%m/%d/%y";
			goto recur;
		case 'd':	/* month day */
		case 'e':
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* [1,31] */
				goto number;
			if (num == 0)
			{
				/*
				* Only %Ed, %Ee permit alternate zero pad.
				*/
				if (flag == 'O')
					goto number;
				return 0;
			}
			if (num > DAY_MAXMON)
				return 0;
			rp->t.tm_mday = num;
			break;
		case 'E':	/* era-specific flag */
			if (flag == '\0')
				goto regchar;
			if (era != 0)
			{
				flag = 'E';
				if (plt->lastnum < 0)
					(void)_era_info(plt, (struct tm *)0);
			}
			continue;
		case 'H':	/* hour (24 hour clock) */
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* [0,23] */
				goto number;
			if (num >= HOUR_DAY)
				return 0;
			rp->t.tm_hour = num;
			break;
		case 'I':	/* hour (12 hour clock) */
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* [1,12] */
				goto number;
			if (num == 0 || num > HOUR_DAY / 2)
				return 0;
			if (rp->t.tm_hour < 0)
				rp->t.tm_hour = num;
			else if ((rp->t.tm_hour += num) >= HOUR_DAY)
				rp->t.tm_hour -= HOUR_DAY;
			break;
		case 'j':	/* year day */
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* [1,366] */
				goto number;
			if (num == 0 || num > DAY_MINYEAR + 1)
				return 0;
			rp->t.tm_yday = num - 1;
			break;
		case 'm':	/* month */
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* [1,12] */
				goto number;
			if (num == 0 || num > MON_YEAR)
				return 0;
			rp->t.tm_mon = num - 1;
			break;
		case 'M':	/* minute */
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* [0,59] */
				goto number;
			if (num >= MIN_HOUR)
				return 0;
			rp->t.tm_min = num;
			break;
		case 'N':	/* date(1)'s default format */
			if (flag == '\0')
				goto regchar;
			str = plt->datecmd;
			goto recur;
		case 'O':	/* alternate number string flag */
			if (flag == '\0')
				goto regchar;
			if (era != 0)
			{
				if (plt->lastnum > 0)
					flag = 'O';
				else if (plt->lastnum < 0)
				{
					(void)_era_info(plt, (struct tm *)0);
					if (plt->lastnum != 0)
						flag = 'O';
				}
			}
			continue;
		case 'p':	/* locale AM/PM equivalent */
			if (flag == '\0')
				goto regchar;
			if ((src = matchstr(&num, src, 2, plt->ampm)) == 0)
				return 0;
			if (num == 0)	/* AM */
			{
				if (rp->t.tm_hour < 0)
					rp->t.tm_hour = 0;
				else if (rp->t.tm_hour > HOUR_DAY / 2)
					rp->t.tm_hour -= HOUR_DAY / 2;
			}
			else if (rp->t.tm_hour < 0)
				rp->t.tm_hour = HOUR_DAY / 2;
			else if ((rp->t.tm_hour += HOUR_DAY / 2) >= HOUR_DAY)
				rp->t.tm_hour -= HOUR_DAY;
			break;
		case 'r':	/* locale AM/PM time format */
			if (flag == '\0')
				goto regchar;
			str = plt->ampmfmt;
			goto recur;
		case 'R':
			if (flag == '\0')
				goto regchar;
			str = "%H:%M";
			goto recur;
		case 'S':	/* seconds */
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* [0,61] */
				goto number;
			if (num >= SEC_MIN + 2)	/* leap seconds! */
				return 0;
			rp->t.tm_sec = num;
			break;
		case 'T':
			if (flag == '\0')
				goto regchar;
			str = "%H:%M:%S";
			goto recur;
		case 'u':	/* weekday with Sunday as 7 */
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* [1,7] */
				goto number;
			if (num == 0 || num > DAY_WEEK)
				return 0;
			if (num == DAY_WEEK)
				num = 0;
			rp->t.tm_wday = num;
			break;
		case 'U':	/* week with Sunday as start */
		case 'V':	/* week with Monday as start */
		case 'W':	/* week with Monday as start */
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* U,W:[0,53]; V:[1,53] */
				goto number;
			if (num > DAY_MINYEAR / DAY_WEEK + 1)
				return 0;
			if (*fmt == 'V' && num == 0)
				return 0;
			rp->week = num;
			rp->code = *fmt;
			break;
		case 'w':	/* weekday with Sunday as 0 */
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* [0,6] */
				goto number;
			if (num >= DAY_WEEK)
				return 0;
			rp->t.tm_wday = num;
			break;
		case 'x':	/* locale's date format */
			if (flag == '\0')
				goto regchar;
			if (flag == 'E')
				str = era->eradatefmt;
			else
				str = plt->datefmt;
			goto recur;
		case 'X':	/* locale's time format */
			if (flag == '\0')
				goto regchar;
			if (flag == 'E')
				str = era->eratimefmt;
			else
				str = plt->timefmt;
			goto recur;
		case 'y':	/* year of century or offset from era start */
			if (flag == '\0')
				goto regchar;
			if (num < 0)	/* [0,99] or [0,???] */
				goto number;
			if (flag != '%')
				rp->year = num;
			else if (num >= 100)
				return 0;
			else if (rp->t.tm_year < 0)
				rp->t.tm_year = BASE_YEAR + num;
			else
				rp->t.tm_year = rp->t.tm_year / 100 * 100 + num;
			break;
		case 'Y':	/* year (with century) or full era year */
			if (flag == '\0')
				goto regchar;
			if (flag == 'E')
			{
				if ((src = erafmt(src, rp)) == 0)
					return 0;
				break;
			}
			if (num < 0)	/* [0,INT_MAX] */
				goto number;
			rp->t.tm_year = num;
			break;
		}
		flag = '\0';
		num = -1;
	}
}

static int
#ifdef __STDC__
jan1wday(long yr)
#else
jan1wday(yr)long yr;
#endif
{
	yr = (yr - EPOCH_YEAR) * DAY_MINYEAR + _nlday(yr);
	(void)_ldivrem(&yr, DAY_WEEK, EPOCH_WDAY);
	return yr;
}

static int
#ifdef __STDC__
delayed(struct recur *rp)
#else
delayed(rp)struct recur *rp;
#endif
{
	long wd;

	if (rp->year >= 0)	/* %Ey or %Oy matched */
	{
		if (rp->ep == 0)
			return 0; /* need to know which era */
		if ((rp->ep->flags & (ERA_REVERSE | ERA_INFINITY))
			== ERA_REVERSE)
		{
			rp->t.tm_year = rp->ep->after.year - rp->year;
		}
		else
		{
			rp->t.tm_year = rp->year + rp->ep->start.year;
		}
		rp->t.tm_year -= rp->ep->offset - BASE_YEAR;
	}
	if (rp->code != '\0')	/* matched a "week number" */
	{
		/*
		* This code only takes a week number as
		* useful if tm_year and tm_wday are known.
		*/
		if (rp->t.tm_year < 0 && rp->year < 0 || rp->t.tm_wday < 0)
			return 0;
		/*
		* Determine the weekday for 1 Jan of tm_year.
		*/
		wd = jan1wday(rp->t.tm_year);
		/*
		* Adjust week number for %V depending on the size
		* of the initial week of the year.
		*/
		if (rp->code == 'V')
		{
			if (wd > 4)
			{
				if (rp->week == 53)
					rp->week = 0;
			}
			else if (wd > 1)
			{
				rp->week--;
			}
		}
		rp->week *= DAY_WEEK;
		rp->week += rp->t.tm_wday - wd;
		if (rp->code != 'U' && rp->t.tm_wday == 0)
			rp->week += DAY_WEEK;
		rp->t.tm_yday = rp->week;
	}
	return 1;
}

static void
#ifdef __STDC__
implied(struct recur *rp)
#else
implied(rp)struct recur *rp;
#endif
{
	/*
	* Need valid year to derive other parts.
	*/
	if (rp->t.tm_year < 0 && rp->year < 0)
		return;
	/*
	* With tm_mon and tm_mday, can determine tm_yday.
	*/
	if (rp->t.tm_mon >= 0 && rp->t.tm_mday >= 0)
	{
		if (rp->t.tm_yday < 0)
		{
			rp->t.tm_yday = _tm_cum_day_mon[rp->t.tm_mon]
				+ rp->t.tm_mday;
			if (rp->t.tm_mon > 1 && ISLEAPYEAR(rp->t.tm_year))
				rp->t.tm_yday++;
		}
	}
	/*
	* With tm_yday, can determine tm_wday, tm_mon, and tm_mday.
	*/
	if (rp->t.tm_yday >= 0)
	{
		if (rp->t.tm_wday < 0)
		{
			rp->t.tm_wday = (rp->t.tm_yday
				+ jan1wday(rp->t.tm_year)) % DAY_WEEK;
		}
		if (rp->t.tm_mon < 0 || rp->t.tm_mday < 0)
		{
			int mon, mday;

			if (rp->t.tm_yday < DAY_JAN)
			{
				mon = 0;
				mday = 1 + rp->t.tm_yday;
			}
			else if (rp->t.tm_yday < DAY_JAN + DAY_MINFEB)
			{
				mon = 1;
				mday = 1 + rp->t.tm_yday - DAY_JAN;
			}
			else
			{
				mon = 1;
				mday = rp->t.tm_yday - DAY_JAN + DAY_MINFEB;
				if (ISLEAPYEAR(rp->t.tm_year) && --mday < 0)
				{
					mday = 1 + DAY_MINFEB;
				}
				else
				{
					while (_tm_day_mon[++mon] <= mday)
						mday -= _tm_day_mon[mon];
					mday++;
				}
			}
			if (rp->t.tm_mon < 0)
				rp->t.tm_mon = mon;
			if (rp->t.tm_mday < 0)
				rp->t.tm_mday = mday;
		}
	}
}

char *
#ifdef __STDC__
strptime(const char *src, const char *fmt, struct tm *ptm)
#else
strptime(src, fmt, ptm)const char *src, *fmt; struct tm *ptm;
#endif
{
	struct recur top;

	if ((top.plt = _lc_time()) == 0)
		return 0;
	top.t.tm_sec = -1;
	top.t.tm_min = -1;
	top.t.tm_hour = -1;
	top.t.tm_mday = -1;
	top.t.tm_mon = -1;
	top.t.tm_year = -1;
	top.t.tm_wday = -1;
	top.t.tm_yday = -1;
	top.ep = 0;
	top.year = -1;
	top.code = '\0';
	if ((src = parse(src, fmt, &top)) != 0)
	{
		if (delayed(&top) == 0)
			src = 0;
		else
		{
			implied(&top);
			/*
			* Assign back only those parts that were touched.
			*/
			if (top.t.tm_sec >= 0)
				ptm->tm_sec = top.t.tm_sec;
			if (top.t.tm_min >= 0)
				ptm->tm_min = top.t.tm_min;
			if (top.t.tm_hour >= 0)
				ptm->tm_hour = top.t.tm_hour;
			if (top.t.tm_mday >= 0)
				ptm->tm_mday = top.t.tm_mday;
			if (top.t.tm_mon >= 0)
				ptm->tm_mon = top.t.tm_mon;
			if (top.t.tm_year >= 0 || top.year >= 0)
				ptm->tm_year = top.t.tm_year - BASE_YEAR;
			if (top.t.tm_wday >= 0)
				ptm->tm_wday = top.t.tm_wday;
			if (top.t.tm_yday >= 0)
				ptm->tm_yday = top.t.tm_yday;
		}
	}
	STDUNLOCK(top.plt->lockptr);
	return (char *)src;
}

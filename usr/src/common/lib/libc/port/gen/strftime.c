/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/strftime.c	1.26"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <limits.h>
#include <stdlib.h>
#include <time.h>
#include "timem.h"

struct recur	/* shared by all instances of strfmt() */
{
	struct tm		t;	/* the time to be formatting */
	struct tz_info		tzbuf;	/* needed to set .tz */
	struct lc_time		*plt;	/* current locale's strings */
	struct era_info		*ep;	/* the era that matches .t */
	size_t			max;	/* remaining space */
	char			*tz;	/* matching timezone name */
};

static char *
#ifdef __STDC__
strfmt(char *dst, const char *fmt, struct recur *rp)	/* real strftime */
#else
strfmt(dst, fmt, rp)char *dst; const char *fmt; struct recur *rp;
#endif
{
	char numstr[sizeof(int) * CHAR_BIT / 3]; /* room for any base 10 */
	struct lc_time *plt = rp->plt;
	struct lc_time_era *era = plt->extra;
	const char *src;
	int flag, val, ndig;
	size_t n;

	for (flag = '\0';; fmt++)
	{
		switch (*fmt)
		{
			/*
			* Use src as a new format to drive further
			* additions to dst.
			*/
		recur:;
			if ((dst = strfmt(dst, src, rp)) == 0)
				return 0;
			flag = '\0';
			continue;
			/*
			* Generate a numeric value to copy into dst.
			* Use alternate number strings if 'O' modifier
			* was specified.  Generate at least ndig digits,
			* but always convert entire value.
			*/
		mk2num:;
			ndig = 2;
		mknum:;
			if (val < 0)
			{
				if (rp->max == 0)
					return 0;
				rp->max--;
				*dst++ = '-';
				val = -val; /* assume not 2's comp. INT_MIN */
			}
			if (flag == 'O' && plt->lastnum > val)
			{
				src = era->altnum[val];
			}
			else
			{
				char *p = &numstr[sizeof(numstr) - 1];

				*p = '\0';
				do
				{
					*--p = val % 10 + '0';
					val /= 10;
				} while (--ndig > 0 || val != 0);
				src = p;
			}
			break;
			/*
			* Copy characters until reach rp->max, \0 or %.
			*/
		default:
		regchar:;
			if (flag != '\0') /* recover entire "bad" sequence */
			{
				if (*--fmt != '%')
					fmt--;	/* at most 2 back */
			}
			do
			{
				if (rp->max == 0)
					return 0;
				*dst++ = *fmt++;
				if (*fmt == '\0')
					return dst;
			} while (--rp->max, *fmt != '%');
			flag = '%';
			continue;
		case '%':
			if (flag != '\0')
			{
				flag = '\0';
				goto regchar;	/* emit single % */
			}
			flag = '%';
			continue;
		case '\0':
			return dst;
		case 'a':	/* abbr. weekday name */
			if (flag == '\0')
				goto regchar;
			src = plt->abday[rp->t.tm_wday];
			break;
		case 'A':	/* weekday name */
			if (flag == '\0')
				goto regchar;
			src = plt->day[rp->t.tm_wday];
			break;
		case 'h':
		case 'b':	/* abbr. month name */
			if (flag == '\0')
				goto regchar;
			src = plt->abmon[rp->t.tm_mon];
			break;
		case 'B':	/* month name */
			if (flag == '\0')
				goto regchar;
			src = plt->mon[rp->t.tm_mon];
			break;
		case 'c':	/* default format string */
			if (flag == '\0')
				goto regchar;
			if (flag == 'E')
				src = era->erabothfmt;
			else
				src = plt->bothfmt;
			goto recur;
		case 'C':	/* century number or era name */
			if (flag == '\0')
				goto regchar;
			if (flag == 'E' && rp->ep != 0)
			{
				src = rp->ep->name;
				break;
			}
			if (rp->t.tm_year < 0)
			{
				val = (rp->t.tm_year + BASE_YEAR) / 100;
				/*
				* Behave to match the expectations of a
				* stupid test suite!  Fortunately, no
				* real application will reach this stuff!
				*/
				if (val < 0)
					val = -val;
			}
			else /* this assumes BASE_YEAR is a multiple of 100 */
				val = rp->t.tm_year / 100 + BASE_YEAR / 100;
			goto mk2num;
		case 'd':	/* 2-digit month day */
			if (flag == '\0')
				goto regchar;
			/*
			* Only %Ed gets an alternate 0 pad.
			*/
			if ((val = rp->t.tm_mday) < 10
				&& flag == 'O' && plt->lastnum > val)
			{
				src = era->altnum[0];
				if ((n = strlen(src)) > rp->max)
					return 0;
				(void)strcpy(dst, src);
				rp->max -= n;
				dst += n;
				src = era->altnum[val];
				break;
			}
			goto mk2num;
		case 'D':
			if (flag == '\0')
				goto regchar;
			src = "%m/%d/%y";
			goto recur;
		case 'e':	/* month day, space padded */
			if (flag == '\0')
				goto regchar;
			if ((val = rp->t.tm_mday) < 10)
			{
				if (rp->max == 0)
					return 0;
				rp->max--;
				*dst++ = ' ';
			}
			ndig = 0;
			goto mknum;
		case 'E':	/* era-specific flag */
			if (flag == '\0')
				goto regchar;
			if (era != 0)
			{
				flag = 'E';
				if (rp->ep == 0 && plt->lastnum != 0)
					rp->ep = _era_info(plt, &rp->t);
			}
			continue;
		case 'H':	/* 2 digit hour (24 hour clock) */
			if (flag == '\0')
				goto regchar;
			val = rp->t.tm_hour;
			goto mk2num;
		case 'I':	/* 2 digit hour (12 hour clock) */
			if (flag == '\0')
				goto regchar;
			if ((val = rp->t.tm_hour % (HOUR_DAY / 2)) == 0)
				val = HOUR_DAY / 2;
			goto mk2num;
		case 'j':	/* 3 digit year day */
			if (flag == '\0')
				goto regchar;
			val = rp->t.tm_yday + 1;
			ndig = 3;
			goto mknum;
		case 'm':	/* 2 digit month */
			if (flag == '\0')
				goto regchar;
			val = rp->t.tm_mon + 1;
			goto mk2num;
		case 'M':	/* 2 digit minute */
			if (flag == '\0')
				goto regchar;
			val = rp->t.tm_min;
			goto mk2num;
		case 'n':
			if (flag == '\0')
				goto regchar;
			src = "\n";
			break;
		case 'N':	/* date(1)'s default format */
			if (flag == '\0')
				goto regchar;
			src = plt->datecmd;
			goto recur;
		case 'O':	/* alternate number string flag */
			if (flag == '\0')
				goto regchar;
			if (era != 0)
			{
				if (rp->ep == 0 && plt->lastnum != 0)
					rp->ep = _era_info(plt, &rp->t);
				if (plt->lastnum > 0)
					flag = 'O';
			}
			continue;
		case 'p':	/* locale AM/PM equivalent */
			if (flag == '\0')
				goto regchar;
			src = plt->ampm[rp->t.tm_hour >= HOUR_DAY / 2];
			break;
		case 'r':	/* locale AM/PM time format */
			if (flag == '\0')
				goto regchar;
			src = plt->ampmfmt;
			goto recur;
		case 'R':
			if (flag == '\0')
				goto regchar;
			src = "%H:%M";
			goto recur;
		case 'S':	/* 2 digit seconds */
			if (flag == '\0')
				goto regchar;
			val = rp->t.tm_sec;
			goto mk2num;
		case 't':
			if (flag == '\0')
				goto regchar;
			src = "\t";
			break;
		case 'T':
			if (flag == '\0')
				goto regchar;
			src = "%H:%M:%S";
			goto recur;
		case 'u':	/* weekday [1,7] with Monday as 1 */
			if (flag == '\0')
				goto regchar;
			if ((val = rp->t.tm_wday) == 0)
				val = DAY_WEEK;
			ndig = 0;
			goto mknum;
		case 'U':	/* 2 digit week with Sunday as start */
			if (flag == '\0')
				goto regchar;
			val = (rp->t.tm_yday + DAY_WEEK - rp->t.tm_wday)
				/ DAY_WEEK;
			goto mk2num;
		case 'V':
			/*
			* 2 digit week with Monday as start.
			* Depends on the number of days of the current week
			* at the beginning of the year:  Too few causes the
			* first days of the year to be taken as the last week
			* of the previous year.
			*/
			if (flag == '\0')
				goto regchar;
			if ((val = 1 + DAY_WEEK - rp->t.tm_wday) > DAY_WEEK)
				val = 1;
			val = (rp->t.tm_yday + val) / DAY_WEEK;
			switch (((DAY_WEEK - (rp->t.tm_yday % DAY_WEEK))
				+ rp->t.tm_wday) % DAY_WEEK)
			{
			case 5:
				if (rp->t.tm_yday <= 1)	/* in previous year */
					val = 53;
				break;
			case 6:
				if (rp->t.tm_yday == 0)	/* in previous year */
					val = 53;
				break;
			case 4:
			case 3:
			case 2:
				val++;
				break;
			}
			goto mk2num;
		case 'w':	/* weekday with Sunday as 0 */
			if (flag == '\0')
				goto regchar;
			val = rp->t.tm_wday;
			ndig = 0;
			goto mknum;
		case 'W':	/* 2 digit week with Monday as start */
			if (flag == '\0')
				goto regchar;
			if ((val = 1 + DAY_WEEK - rp->t.tm_wday) > DAY_WEEK)
				val = 1;
			val = (rp->t.tm_yday + val) / DAY_WEEK;
			goto mk2num;
		case 'x':	/* locale's date format */
			if (flag == '\0')
				goto regchar;
			if (flag == 'E')
				src = era->eradatefmt;
			else
				src = plt->datefmt;
			goto recur;
		case 'X':	/* locale's time format */
			if (flag == '\0')
				goto regchar;
			if (flag == 'E')
				src = era->eratimefmt;
			else
				src = plt->timefmt;
			goto recur;
		case 'y':	/* year of century or offset from era start */
			if (flag == '\0')
				goto regchar;
			if (flag != '%' && rp->ep != 0)
			{
				struct era_info *ep = rp->ep;

				/*
				* This code assumes that both %Oy and %Ey
				* are to be the offset from the era.
				*/
				val = rp->t.tm_year + ep->offset;
				if ((ep->flags & (ERA_REVERSE | ERA_INFINITY))
					== ERA_REVERSE)
				{
					val = ep->after.year - val;
				}
				else
				{
					val -= ep->start.year;
				}
			}
			else /* assumes that BASE_YEAR % 100 == 0 */
			{
				val = rp->t.tm_year % 100;
			}
			goto mk2num;
		case 'Y':	/* year (with century) or full era year */
			if (flag == '\0')
				goto regchar;
			if (flag == 'E' && rp->ep != 0)
			{
				src = rp->ep->fmt;
				goto recur;
			}
			val = rp->t.tm_year + BASE_YEAR;
			ndig = 0;
			goto mknum;
		case 'Z':	/* timezone name */
			if (flag == '\0')
				goto regchar;
			if (rp->tz == 0)
			{
				_tz_info(&rp->tzbuf);
				if (rp->tzbuf.str[0][0] == '\0')
				{
					struct tm tmbuf;

					tmbuf = rp->t;
					(void)_tz_file(&rp->tzbuf,
						_norm_tm(&tmbuf),
						rp->t.tm_isdst + 1);
				}
				rp->tz = rp->tzbuf.str[rp->t.tm_isdst > 0];
			}
			src = rp->tz;
			break;
		}
		/*
		* Copy from src to str.
		*/
		if ((n = strlen(src)) > rp->max)
			return 0;
		(void)strcpy(dst, src);
		rp->max -= n;
		dst += n;
		flag = '\0';
	}
}

size_t
#ifdef __STDC__
strftime(char *dst, size_t max, const char *fmt, const struct tm *ptm)
#else
strftime(dst,max,fmt,ptm)char*dst;size_t max;const char*fmt;const struct tm*ptm;
#endif
{
	struct recur top;
	char *str;

#ifdef CALL_TZSET
	tzset();
#endif
	if (max == 0 || (top.plt = _lc_time()) == 0)
		return 0;
	/*
	* Hopefully, the cost of copying the tm structure is made
	* up by the number of indirections not done in strfmt().
	*/
	top.t = *ptm;
	top.ep = 0;
	top.max = max - 1;	/* room for \0 */
	top.tz = 0;
	if (fmt == 0)
		fmt = top.plt->bothfmt;
	if ((str = strfmt(dst, fmt, &top)) == 0)
		str = dst;
	*str = '\0';
	STDUNLOCK(top.plt->lockptr);
	return str - dst;
}

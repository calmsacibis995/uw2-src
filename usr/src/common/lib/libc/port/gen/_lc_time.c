/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_lc_time.c	1.4"

#include "synonyms.h"
#include <locale.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include "timem.h"
#include "_locale.h"

static const char *const locale_c[] = /* C locale strings in file order */
{
	_str_abmon[0],	_str_abmon[1],	_str_abmon[2],	_str_abmon[3],
	_str_abmon[4],	_str_abmon[5],	_str_abmon[6],	_str_abmon[7],
	_str_abmon[8],	_str_abmon[9],	_str_abmon[10],	_str_abmon[11],
	"January",	"February",	"March",	"April",
	"May",		"June",		"July",		"August",
	"September",	"October",	"November",	"December",
	_str_abday[0],	_str_abday[1],	_str_abday[2],	_str_abday[3],
	_str_abday[4],	_str_abday[5],	_str_abday[6],
	"Sunday",	"Monday",	"Tuesday",	"Wednesday",
	"Thursday",	"Friday",	"Saturday",
	"%H:%M:%S",	"%m/%d/%y",	"%a %b %e %H:%M:%S %Y",
	"AM",	"PM",	"%a %b %e %T %Z %Y",	"%I:%M:%S %p",
};

static const short offset[] = /* maps strings in file to structure */
{
	offsetof(struct lc_time, abmon[0]),
	offsetof(struct lc_time, abmon[1]),
	offsetof(struct lc_time, abmon[2]),
	offsetof(struct lc_time, abmon[3]),
	offsetof(struct lc_time, abmon[4]),
	offsetof(struct lc_time, abmon[5]),
	offsetof(struct lc_time, abmon[6]),
	offsetof(struct lc_time, abmon[7]),
	offsetof(struct lc_time, abmon[8]),
	offsetof(struct lc_time, abmon[9]),
	offsetof(struct lc_time, abmon[10]),
	offsetof(struct lc_time, abmon[11]),
	offsetof(struct lc_time, mon[0]),
	offsetof(struct lc_time, mon[1]),
	offsetof(struct lc_time, mon[2]),
	offsetof(struct lc_time, mon[3]),
	offsetof(struct lc_time, mon[4]),
	offsetof(struct lc_time, mon[5]),
	offsetof(struct lc_time, mon[6]),
	offsetof(struct lc_time, mon[7]),
	offsetof(struct lc_time, mon[8]),
	offsetof(struct lc_time, mon[9]),
	offsetof(struct lc_time, mon[10]),
	offsetof(struct lc_time, mon[11]),
	offsetof(struct lc_time, abday[0]),
	offsetof(struct lc_time, abday[1]),
	offsetof(struct lc_time, abday[2]),
	offsetof(struct lc_time, abday[3]),
	offsetof(struct lc_time, abday[4]),
	offsetof(struct lc_time, abday[5]),
	offsetof(struct lc_time, abday[6]),
	offsetof(struct lc_time, day[0]),
	offsetof(struct lc_time, day[1]),
	offsetof(struct lc_time, day[2]),
	offsetof(struct lc_time, day[3]),
	offsetof(struct lc_time, day[4]),
	offsetof(struct lc_time, day[5]),
	offsetof(struct lc_time, day[6]),
	offsetof(struct lc_time, timefmt),
	offsetof(struct lc_time, datefmt),
	offsetof(struct lc_time, bothfmt),
	offsetof(struct lc_time, ampm[0]),
	offsetof(struct lc_time, ampm[1]),
	offsetof(struct lc_time, datecmd),
	offsetof(struct lc_time, ampmfmt),
};

#define NFIXED	(sizeof(offset) / sizeof(offset[0]))

struct lc_time *
#ifdef __STDC__
_lc_time(void)
#else
_lc_time()
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	static char *curloc;
	static struct lc_time *curinfo;
	struct lc_time_era *ep;
	char *new[NFIXED];
	char *cat, *p;
	int nnew, i;

	STDLOCK(&lock);
	cat = _locale[LC_TIME];
	if (curinfo != 0)
	{
		if (cat == curloc)
			return curinfo;	/* still locked */
	}
	else if ((curinfo =
		(struct lc_time *)malloc(sizeof(struct lc_time))) == 0)
	{
		goto err0;
	}
	else
	{
#ifdef _REENTRANT
		curinfo->lockptr = &lock;
#endif
		curinfo->extra = 0;
	}
	/*
	* Fill in new[] with nnew strings from a non-C locale LC_TIME.
	*/
	ep = 0;
	curinfo->etc = 0;
	curinfo->lastnum = 0;
	nnew = 0;
	if (cat != _str_c)
	{
		struct stat st;
		int fd;

		if ((fd = _openlocale(LC_TIME, cat, (char *)0)) == -1)
			goto err0;
		if (fstat(fd, &st) == -1)
		{
		err1:;
			close(fd);
			goto err0;
		}
		if ((ep = (struct lc_time_era *)malloc(
			sizeof(struct lc_time_era) + st.st_size + 1)) == 0)
		{
			goto err1;
		}
		if (read(fd, (void *)(p = (char *)&ep[1]), st.st_size)
			!= st.st_size)
		{
		err2:;
			free((void *)ep);
			goto err1;
		}
		close(fd);
		p[st.st_size] = '\0';	/* guarantee stringness */
		/*
		* Walk through the initial part of the contents,
		* replacing newlines with \0s and noting the
		* resulting strings.  The list ends when:
		*  - a line containing just a % is reached, or
		*  - no more newlines are found (eof), or
		*  - all fixed strings are found.
		*/
		for (;;)
		{
			new[nnew] = p;
			if (*p == '%' && *++p == '\n')
			{
				p++;
				break;
			}
			if ((p = strchr(p, '\n')) == 0)
				break;
			*p++ = '\0';
			if (++nnew == NFIXED)
			{
				if (p[0] == '%' && p[1] == '\n')
					p += 2;
				break;
			}
		}
		/*
		* Don't process the rest of the file; wait to be
		* asked.  But, let the caller know if there is
		* more information available.
		*/
		if (p != 0 && *p != '\0')
		{
			curinfo->etc = p;
			curinfo->lastnum = -1;
		}
	}
	curloc = cat;
	/*
	* Fill in the new strings, using the C locale's
	* for any absent ones.
	*/
	i = 0;
	do
	{
		p = offset[i] + (char *)curinfo;
		if (i < nnew)
			*(char **)p = new[i];
		else
			*(char **)p = (char *)locale_c[i];
	} while (++i < NFIXED);
	/*
	* Toss any old era information.
	*/
	if (curinfo->extra != 0)
	{
		struct era_info *xp, *next;

		for (xp = curinfo->extra->head; xp != 0; xp = next)
		{
			next = xp->next;
			free((void *)xp);
		}
		free((void *)curinfo->extra); /* old strings, too */
	}
	/*
	* Fill in the new defaults, if any.
	* They will be reset by _era_info(), if there's more.
	*/
	if ((curinfo->extra = ep) != 0)
	{
		ep->head = 0;
		ep->eratimefmt = curinfo->timefmt;
		ep->eradatefmt = curinfo->datefmt;
		ep->erabothfmt = curinfo->bothfmt;
	}
	return curinfo;	/* still locked */
err0:;
	STDUNLOCK(&lock);
	return 0;
}

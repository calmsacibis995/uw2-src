/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_era_info.c	1.2"

#include "synonyms.h"
#include <ctype.h>
#include <string.h>
#include <time.h>
#include "timem.h"

static struct era_info *
#ifdef __STDC__
setera(char *p)	/* allocate and fill one era from LC_TIME string */
#else
setera(p)char *p;
#endif
{
	struct era_info *ep;

	if ((ep = (struct era_info *)malloc(sizeof(struct era_info))) == 0)
		return 0;
	if (*p == '-')
		ep->flags = ERA_REVERSE;
	else
		ep->flags = 0;
	if (*++p != ':')
		goto err;
	ep->offset = atoi(++p);
	if ((p = strchr(p, ':')) == 0)
		goto err;
	ep->start.year = atoi(++p) - BASE_YEAR;
	if ((p = strchr(p, '/')) == 0)
		goto err;
	ep->start.mon = atoi(++p) - 1;
	if ((p = strchr(p, '/')) == 0)
		goto err;
	ep->start.mday = atoi(++p);
	if ((p = strchr(p, ':')) == 0)
		goto err;
	if (*++p == '\0')	/* makes the check of p[1] safe */
		goto err;
	if (p[1] == '*')
	{
		ep->flags |= ERA_INFINITY;
		if (*p == '-')
			ep->flags |= ERA_NEGINF;
		if (*(p += 2) != ':')
			goto err;
	}
	else /* full year/mon/day specification */
	{
		ep->after.year = atoi(p) - BASE_YEAR;
		if ((p = strchr(p, '/')) == 0)
			goto err;
		ep->after.mon = atoi(++p) - 1;
		if ((p = strchr(p, '/')) == 0)
			goto err;
		ep->after.mday = atoi(++p);
		if ((p = strchr(p, ':')) == 0)
			goto err;
	}
	ep->name = ++p;
	if ((p = strchr(p, ':')) == 0)
		goto err;
	*p = '\0';
	ep->fmt = &p[1];
	ep->next = 0;
	return ep;
err:;
	free((void *)ep);
	return 0;
}

static int
#ifdef __STDC__
cmpera(const struct tm *p, const struct ymd *q) /* return -1,0,1 as p ? q */
#else
cmpera(p, q)const struct tm *p; const struct ymd *q;
#endif
{
	if (p->tm_year > q->year)
		return 1;
	if (p->tm_year < q->year)
		return -1;
	if (p->tm_mon > (int)q->mon)
		return 1;
	if (p->tm_mon < (int)q->mon)
		return -1;
	if (p->tm_mday > (int)q->mday)
		return 1;
	if (p->tm_mday < (int)q->mday)
		return -1;
	return 0;
}

static char *
#ifdef __STDC__
setstr(char *p, const char **ptr)	/* go past "...", set *ptr to string */
#else
setstr(p, ptr)char *p; const char **ptr;
#endif
{
	while (*p != '"')
	{
		if (*p == '\0')
			return 0;
		p++;
	}
	*ptr = ++p;
	while (*p != '"')
	{
		if (*p == '\0')
			return p;
		p++;
	}
	*p++ = '\0';
	return p;
}

struct era_info *
#ifdef __STDC__
_era_info(struct lc_time *plt, const struct tm *ptm)
#else
_era_info(plt, ptm)struct lc_time *plt; const struct tm *ptm;
#endif
{
	static const char alt_digits[] = "alt_digits";
	static const char t_fmt[] = "t_fmt";
	struct lc_time_era *xp = plt->extra;
	struct era_info **epp = &xp->head;
	char *p;
	char *key;
	int i;

	if ((p = plt->etc) == 0)
		goto eracheck;	/* already filled in the era information */
	/*
	* Set fields to their default values.
	*/
	plt->etc = 0;
	plt->lastnum = 0;
	/*
	* Fill in all values present.
	* Each entry starts with a key and is followed by at least
	* one double-quote-bracketed string.  If the entry accepts
	* a list of strings, they are separated by semicolons that
	* immediately follow each string.
	*/
	do
	{
		while (!islower(*p))
		{
			if (!isspace(*p++))
				goto eracheck;
		}
		key = p;
		while (islower(*p) || *p == '_')
			p++;
		*p++ = '\0';
		/*
		* Match key against (in order) "alt_digits",
		* "era", "era_t_fmt", "era_d_t_fmt", and "era_d_fmt".
		*/
		if (key[0] != 'e' || key[1] != 'r' || key[2] != 'a')
		{
			/*
			* Must be "alt_digits".
			*/
			if (strcmp(key, alt_digits) != 0)
				break;
			i = 0;
			do
			{
				if ((p = setstr(p, &xp->altnum[i])) == 0)
					break;
			} while (++i < MAXALTNUM && *p == ';');
			plt->lastnum = i;
			if (p == 0)
				break;
		}
		else if (key[3] == '\0')	/* "era" */
		{
			do
			{
				if ((p = setstr(p, (const char **)&key)) == 0)
					goto eracheck;
				if ((*epp = setera(key)) == 0)
					goto eracheck;
				epp = &(*epp)->next;
			} while (*p == ';');
		}
		else if (key[3] != '_')
		{
			break;
		}
		else if (key[4] != 'd')
		{
			/*
			* Must be "era_t_fmt".
			*/
			if (strcmp(&key[4], t_fmt) != 0)
				break;
			if ((p = setstr(p, &xp->eratimefmt)) == 0)
				break;
		}
		else if (key[5] != '_')
		{
			break;
		}
		else if (key[6] != 'f')
		{
			/*
			* Must be "era_d_t_fmt".
			*/
			if (strcmp(&key[6], t_fmt) != 0)
				break;
			if ((p = setstr(p, &xp->erabothfmt)) == 0)
				break;
		}
		else if (key[7] != 'm' || key[8] != 't' || key[9] != '\0')
		{
			break;
		}
		else if ((p = setstr(p, &xp->eradatefmt)) == 0)
		{
			break;
		}
	} while (*p != '\0');
eracheck:;
	if (ptm != 0)
	{
		struct era_info *ep;

		/*
		* Determine the era that matches the date, if any.
		*/
		for (ep = xp->head; ep != 0; ep = ep->next)
		{
			i = cmpera(ptm, &ep->start);
			if (ep->flags & ERA_NEGINF)
			{
				if (i <= 0)
					break;
			}
			else if (ep->flags & ERA_INFINITY)
			{
				if (i >= 0)
					break;
			}
			else if (i >= 0 && cmpera(ptm, &ep->after) <= 0)
				break;
		}
		return ep;
	}
	return 0;
}

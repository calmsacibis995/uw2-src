/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/nl_langinfo.c	1.15"

#include "synonyms.h"
#include <limits.h>
#include <langinfo.h>
#include <locale.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include "_locale.h"
#include "stdlock.h"
#include "timem.h"

#ifdef __STDC__
	#pragma weak nl_langinfo = _nl_langinfo
#endif

#define UNCLEARED_ALLOC
#define VARYING_ALLOC
#include "statalloc.h"

static char *
#ifdef __STDC__
getbuf(int n)
#else
getbuf(n)int n;
#endif
{
	char *p;

	STATALLOC(p, char, n, return 0;);
	return p;
}

static char *
#ifdef __STDC__
currency(void)
#else
currency()
#endif
{
	struct lconv *plc = localeconv();
	char *s, *p;
	int ch;

	if (*(p = plc->currency_symbol) == '\0')
	{
		if (*(p = plc->mon_decimal_point) == '\0')
			return 0;
		ch = '.';
	}
	else if (plc->p_cs_precedes == 0)
		ch = '+';
	else if (plc->p_cs_precedes == 1)
		ch = '-';
	else
		return 0;
	if ((s = getbuf(strlen(p) + 2)) == 0)
		return 0;
	s[0] = ch;
	(void)strcpy(&s[1], p);
	return s;
}

static char *
#ifdef __STDC__
msgstr(int id, const char *dflt)
#else
msgstr(id, dflt)int id; const char *dflt;
#endif
{
	static const char Xopen[] = "Xopen_info";
	char *s;

	if ((s = (char *)_g1txt(_locale[LC_CTYPE], Xopen, id)) == 0)
		s = (char *)dflt;
	return s;
}

static char *
#ifdef __STDC__
altdigs(struct lc_time *plt)
#else
altdigs(plt)struct lc_time *plt;
#endif
{
	struct lc_time_era *plte;
	const char **pp;
	char *s, *p;
	int i, k, n;

	if ((i = plt->lastnum) == 0 || (plte = plt->extra) == 0)
		return 0;
	n = i << 2;	/* initial buffer size guess */
	if ((s = getbuf(n)) == 0)
		return 0;
	pp = &plte->altnum[0];
	p = s;
	for (;;) /* build ;-separated list */
	{
		if ((k = strlen(*pp)) >= n)	/* need to grow */
		{
			int used = p - s;

			n = (used + k) << 1;
			if ((s = getbuf(n)) == 0)
				return 0;
			p = s + used;
			n -= used;
		}
		(void)memcpy((void *)p, (void *)*pp++, (size_t)k);
		k++;
		p += k;
		if (--i == 0)
			break;
		p[-1] = ';';
		n -= k;
	}
	p[-1] = '\0';
	return s;
}

	/*
	* MAXDIG -- upper bound for the number of decimal digits in an int.
	* SZERA -- upper bound for an era entry string:
	*	"%c:%d:%.4d/%.2d/%.2d:%.4d/%.2d/%.2d:%s:%s;"
	*/
#define MAXDIG		(sizeof(int) * CHAR_BIT / 3)	/* 3+ bits/digit */
#define SZERA(ln, lf)	(1 + 1 + MAXDIG + \
				2 * (1 + MAXDIG + 1 + 2 + 1 + 2) + \
				1 + (ln) + 1 + (lf) + 1)

static char *
#ifdef __STDC__
erainfo(struct lc_time *plt)
#else
erainfo(plt)struct lc_time *plt;
#endif
{
	struct lc_time_era *plte;
	struct era_info *ep;
	char *s, *p;
	int i, k, n;

	if ((plte = plt->extra) == 0 || (ep = plte->head) == 0)
		return 0;
	n = 500;	/* initial buffer size guess */
	if ((s = getbuf(n)) == 0)
		return 0;
	p = s;
	for (;;)
	{
		if ((k = SZERA(strlen(ep->name), strlen(ep->fmt))) > n)
		{
			int used = p - s;

			n = (used + k) << 1;
			if ((s = getbuf(n)) == 0)
				return 0;
			p = s + used;
			n -= used;
		}
		k = sprintf(p, "%c:%d:%.4d/%.2d/%.2d:",
			(ep->flags & ERA_REVERSE) ? '-' : '+', ep->offset,
			ep->start.year + BASE_YEAR, ep->start.mon + 1,
			ep->start.mday);
		p += k;
		n -= k;
		if (ep->flags & ERA_INFINITY)
		{
			if (ep->flags & ERA_NEGINF)
				p[0] = '-';
			else
				p[0] = '+';
			p[1] = '*';
			k = 2;
		}
		else
		{
			k = sprintf(p, "%.4d/%.2d/%.2d",
				ep->after.year + BASE_YEAR,
				ep->after.mon + 1, ep->after.mday);
		}
		p += k;
		n -= k;
		k = sprintf(p, ":%s:%s", ep->name, ep->fmt);
		p += k;
		if ((ep = ep->next) == 0)
			break;
		*p++ = ';';
		n -= k + 1;
	}
	*p = '\0';
	return s;
}

char *
#ifdef __STDC__
nl_langinfo(nl_item item)
#else
nl_langinfo(item)nl_item item;
#endif
{
	static const char empty[] = "";
	static const char yesstr[] = "yes";
	static const char nostr[] = "no";
	static const char yesexpr[] = "^(y|Y)";
	static const char noexpr[] = "^(n|N)";
	struct lc_time *plt;
	char *s;

	switch (item)	/* handle all items unrelated to LC_TIME */
	{
	case RADIXCHAR:
		if ((s = getbuf(2)) == 0)
			return (char *)empty;
		s[0] = _numeric[0];
		s[1] = '\0';
		return s;
	case THOUSEP:
		if ((s = getbuf(2)) == 0)
			return (char *)empty;
		s[0] = _numeric[1];
		s[1] = '\0';
		return s;
	case CRNCYSTR:
		if ((s = currency()) == 0)
			s = (char *)empty;
		return s;
	case YESSTR:
		return msgstr(4, yesstr);
	case NOSTR:
		return msgstr(5, nostr);
	case YESEXPR:
		return msgstr(6, yesexpr);
	case NOEXPR:
		return msgstr(7, noexpr);
	case CODESET:
		return msgstr(9, empty);
	}
	if ((plt = _lc_time()) == 0)
		return (char *)empty;
	if (plt->lastnum < 0)
		(void)_era_info(plt, (struct tm *)0);
	switch (item)
	{
	default:
		s = (char *)empty;
		break;
	case ABMON_1:
	case ABMON_2:
	case ABMON_3:
	case ABMON_4:
	case ABMON_5:
	case ABMON_6:
	case ABMON_7:
	case ABMON_8:
	case ABMON_9:
	case ABMON_10:
	case ABMON_11:
	case ABMON_12:
		s = (char *)plt->abmon[item - ABMON_1];
		break;
	case MON_1:
	case MON_2:
	case MON_3:
	case MON_4:
	case MON_5:
	case MON_6:
	case MON_7:
	case MON_8:
	case MON_9:
	case MON_10:
	case MON_11:
	case MON_12:
		s = (char *)plt->mon[item - MON_1];
		break;
	case ABDAY_1:
	case ABDAY_2:
	case ABDAY_3:
	case ABDAY_4:
	case ABDAY_5:
	case ABDAY_6:
	case ABDAY_7:
		s = (char *)plt->abday[item - ABDAY_1];
		break;
	case DAY_1:
	case DAY_2:
	case DAY_3:
	case DAY_4:
	case DAY_5:
	case DAY_6:
	case DAY_7:
		s = (char *)plt->day[item - DAY_1];
		break;
	case AM_STR:
	case PM_STR:
		s = (char *)plt->ampm[item - AM_STR];
		break;
	case T_FMT_AMPM:
		s = (char *)plt->ampmfmt;
		break;
	case ERA_T_FMT:
		if (plt->extra != 0)
		{
			s = (char *)plt->extra->eratimefmt;
			break;
		}
		/*FALLTHROUGH*/
	case T_FMT:
		s = (char *)plt->timefmt;
		break;
	case ERA_D_FMT:
		if (plt->extra != 0)
		{
			s = (char *)plt->extra->eradatefmt;
			break;
		}
		/*FALLTHROUGH*/
	case D_FMT:
		s = (char *)plt->datefmt;
		break;
	case ERA_D_T_FMT:
		if (plt->extra != 0)
		{
			s = (char *)plt->extra->erabothfmt;
			break;
		}
		/*FALLTHROUGH*/
	case D_T_FMT:
		s = (char *)plt->bothfmt;
		break;
	case ALT_DIGITS:
		if ((s = altdigs(plt)) == 0)
			s = (char *)empty;
		break;
	case ERA:
		if ((s = erainfo(plt)) == 0)
			s = (char *)empty;
		break;
	}
	STDUNLOCK(plt->lockptr);
	return s;
}

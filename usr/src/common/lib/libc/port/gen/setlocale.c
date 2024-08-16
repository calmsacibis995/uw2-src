/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/setlocale.c	1.18"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <locale.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "stdlock.h"
#include "_locale.h"

static const char okay[] = "";	/* causes _openlocale to access() only */

static int
#ifdef __STDC__
update(int cat, const char *loc)	/* _locale[cat] = loc */
#else
update(cat, loc)int cat; const char *loc;
#endif
{
	static struct item
	{
		struct item	*next;
		char		str[1];
	} *list;
	register struct item **ipp, *ip;

	/*
	* Manage a permanent set of unique strings.  This guarantees
	* that the rest of the locale-dependent code in libc can use
	* simple pointer comparisons instead of strcmp()s, and that
	* a string will not change after being added to _locale[].
	*
	* The following implementation is not suitable for heavy use.
	* Thus, we assume that the typical application uses very few
	* unique locale string values.
	*
	* First check for the "C" special case.
	* Otherwise, search through a singly linked list of strings.
	* When a new item is added or an existing entry is matched,
	* that item is placed at the head of the list.  This dynamic
	* reorganization keeps search times smaller, in practice.
	*/
	if (loc == _str_c || loc[0] == 'C' && loc[1] == '\0')
	{
		_locale[cat] = (char *)_str_c;
		return 0;
	}
	for (ipp = &list;; ipp = &ip->next)
	{
		if ((ip = *ipp) != 0)
		{
			if (loc != ip->str && strcmp(ip->str, loc) != 0)
				continue;	/* doesn't match */
			*ipp = ip->next;
		}
		else if ((ip = (struct item *)malloc(strlen(loc)
			+ offsetof(struct item, str[1]))) == 0)
		{
			return -1;
		}
		else
		{
			(void)strcpy(ip->str, loc);
		}
		_locale[cat] = ip->str;
		ip->next = list;
		list = ip;
		return 0;
	}
}

static int
#ifdef __STDC__
setctype(const char *loc)	/* attempt change to LC_CTYPE */
#else
setctype(loc)const char *loc;
#endif
{
	unsigned char data[SZ_CTYPE + SZ_CODESET];
	int fd, n;

	if ((fd = _openlocale(LC_CTYPE, loc, (char *)0)) == -1)
		return -1;
	n = read(fd, (void *)data, sizeof(data));
	close(fd);
	if (n != sizeof(data) || update(LC_CTYPE, loc) != 0)
		return -1;
	(void)memcpy((void *)_ctype, (void *)data, sizeof(data));
	return 0;
}

static int
#ifdef __STDC__
setnumeric(const char *loc)	/* attempt change to LC_NUMERIC */
#else
setnumeric(loc)const char *loc;
#endif
{
	unsigned char data[SZ_NUMERIC + SZ_GROUPING];
	unsigned char *p, *q;
	struct stat statbuf;
	int fd, ans;

	if ((fd = _openlocale(LC_NUMERIC, loc, (char *)0)) == -1)
		return -1;
	ans = -1;
	p = data;
	if (fstat(fd, &statbuf) != 0)
		goto ret;
	if (statbuf.st_size > sizeof(data))
	{
		if ((p = malloc(statbuf.st_size)) == 0)
			goto ret;
	}
	else if (statbuf.st_size < SZ_NUMERIC)
		goto ret;
	if (read(fd, (void *)p, statbuf.st_size) != statbuf.st_size)
		goto ret;
	if (update(LC_NUMERIC, loc) != 0)
		goto ret;
	ans = 0;
	q = _grouping;
	if (p != data)
		_grouping = &p[SZ_NUMERIC];
	else
	{
		int i;

		_numeric[SZ_NUMERIC] = '\0';	/* clear _grouping first */
		_grouping = &_numeric[SZ_NUMERIC];
		i = statbuf.st_size;
		while (--i >= SZ_NUMERIC)	/* copy in reverse order */
			_numeric[i] = data[i];
	}
	_numeric[0] = p[0];
	_numeric[1] = p[1];		/* assumes that SZ_NUMERIC is 2 */
	if (q != &_numeric[SZ_NUMERIC])	/* previous was allocated */
		free(&q[-SZ_NUMERIC]);
	p = 0;
ret:;
	if (p != 0 && p != data)
		free(p);
	close(fd);
	return ans;
}

static int
#ifdef __STDC__
setone(int cat, const char *loc) /* attempt setting change for one category */
#else
setone(cat, loc)int cat; const char *loc;
#endif
{
	const char *p = loc;

	if (*p == '\0' && ((p = getenv(_str_lc_all)) == 0 || *p == '\0')
		&& ((p = getenv(_str_catname[cat])) == 0 || *p == '\0')
		&& ((p = getenv(_str_lang)) == 0 || *p == '\0'))
	{
		p = _str_c;
	}
	if (strcmp(_locale[cat], p) != 0)
	{
		/*
		* This assumes that LC_CTYPE and LC_NUMERIC are 0 and 1.
		*/
		if (cat <= LC_NUMERIC)
			return cat == LC_NUMERIC ? setnumeric(p) : setctype(p);
		if (_openlocale(cat, p, okay) == -1 || update(cat, p) != 0)
			return -1;
	}
	return 0;
}

#define UNCLEARED_ALLOC
#define VARYING_ALLOC
#include "statalloc.h"

static char *
#ifdef __STDC__
space(size_t n)	/* return buffer at least n bytes long */
#else
space(n)size_t n;
#endif
{
	char *p;

	STATALLOC(p, char, n, return 0;);
	return p;
}

static int
#ifdef __STDC__
setall(const char *loc)	/* attempt setting change to entire locale */
#else
setall(loc)const char *loc;
#endif
{
	char partbuf[LC_ALL * LC_NAMELEN];
	const char *p, *part[LC_ALL];
	int cat;

	/*
	* Fill in part[] with each category's value.
	*/
	if (*loc == ':')	/* composite locale */
	{
		size_t n;
		char *s;

		/*
		* Separate the :-separated list into distinct strings,
		* using allocated space or partbuf[] if it's long enough.
		* Note that the last item in the composite is not copied.
		*/
		s = partbuf;
		if ((n = strrchr(loc, ':') - loc) > sizeof(partbuf))
		{
			if ((s = space(n)) == 0)
				return -1;
		}
		loc++;
		cat = LC_ALL - 1;
		do
		{
			if ((p = strchr(loc, ':')) == 0)
			{
				part[cat] = loc;
			}
			else /* need to copy it; others follow */
			{
				part[cat] = s;
				n = p - loc;
				(void)memcpy((void *)s, (const void *)loc, n);
				s += n;
				*s++ = '\0';
				loc = p + 1;
			}
		} while (--cat >= 0);
	}
	else /* single locale, or "" (which uses environment variables) */
	{
		const char *def;

		if (*loc == '\0'
			&& ((loc = getenv(_str_lc_all)) == 0 || *loc == '\0'))
		{
			loc = 0;
			if ((def = getenv(_str_lang)) == 0 || *def == '\0')
				def = _str_c;
		}
		cat = LC_ALL - 1;
		do
		{
			if ((p = loc) == 0
				&& ((p = getenv(_str_catname[cat])) == 0
				|| *p == '\0'))
			{
				p = def;
			}
			part[cat] = p;
		} while (--cat >= 0);
	}
	/*
	* Try the entire new locale "on for size", in reverse order.
	* This causes less fuss, in general, for the typical bad locale.
	* This code assumes that LC_CTYPE and LC_NUMERIC are 0 and 1.
	*/
	cat = LC_ALL - 1;
	do
	{
		if (strcmp(_locale[cat], part[cat]) == 0)
		{
			part[cat] = 0;
			continue;
		}
		if (_openlocale(cat, part[cat], okay) == -1)
			return -1;
	} while (--cat > LC_NUMERIC);
	p = 0;
	if (strcmp(_locale[LC_CTYPE], part[LC_CTYPE]) != 0)
	{
		p = _locale[LC_CTYPE];		/* just in case */
		if (setctype(part[LC_CTYPE]) != 0)
			return -1;
	}
	if (strcmp(_locale[LC_NUMERIC], part[LC_NUMERIC]) != 0)
	{
		if (setnumeric(part[LC_NUMERIC]) != 0)
		{
			if (p != 0)
				(void)setctype(p); /* restore LC_CTYPE */
			return -1;
		}
	}
	/*
	* All categories look okay to the extent that we check here.
	* Change _locale[] for other than LC_CTYPE and LC_NUMERIC (0 and 1).
	*/
	cat = LC_ALL - 1;
	do
	{
		if (part[cat] != 0 && update(cat, part[cat]) != 0)
			return -1;
	} while (--cat > LC_NUMERIC);
	return 0;
}

char *
#ifdef __STDC__
setlocale(int cat, const char *loc)
#else
setlocale(cat, loc)int cat; const char *loc;
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	char *ans;

	STDLOCK(&lock);
	if (loc != 0 && (cat != LC_ALL ? setone(cat, loc) : setall(loc)) != 0)
	{
		ans = 0;	/* failed to change locale */
	}
	else if (cat != LC_ALL)
	{
		ans = _locale[cat];
	}
	else /* produce a composite locale string */
	{
		size_t n;
		int low;

		/*
		* First determine its length, while noting in low the
		* index of the category that last matches _locale[0].
		*/
		n = 0;
		low = 0;
		cat = 1;
		do
		{
			if (n == 0 && _locale[cat] == _locale[0])
			{
				low = cat;
				continue;
			}
			n += 1 + strlen(_locale[cat]);
		} while (++cat < LC_ALL);
		if (n == 0)
		{
			ans = _locale[0]; /* all categories are the same */
		}
		else if ((ans = space(n + 1 + strlen(_locale[0]) + 1)) != 0)
		{
			char *p = ans;

			/*
			* Put together all unique categories as a
			* :-separated list, until one of the last
			* identical values [low] is added.
			*/
			cat = LC_ALL - 1;
			do
			{
				*p = ':';
				(void)strcpy(++p, _locale[cat]);
				p += strlen(_locale[cat]);
			} while (--cat >= low);
		}
	}
	STDUNLOCK(&lock);
	return ans;
}

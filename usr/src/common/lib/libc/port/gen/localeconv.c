/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/localeconv.c	1.12"

#include "synonyms.h"
#include <limits.h>
#include <locale.h>
#include <string.h>
#include <sys/stat.h>
#include "_locale.h"
#include "stdlock.h"

static const char dot[] = ".";	/* used for all strings in "C" locale */

static const struct lconv locale_c =
{
	(char *)&dot[0], (char *)&dot[1], (char *)&dot[1],
	(char *)&dot[1], (char *)&dot[1],
	(char *)&dot[1], (char *)&dot[1], (char *)&dot[1],
	(char *)&dot[1], (char *)&dot[1],
	CHAR_MAX, CHAR_MAX,
	CHAR_MAX, CHAR_MAX,
	CHAR_MAX, CHAR_MAX,
	CHAR_MAX, CHAR_MAX,
};

struct lconv *
#ifdef __STDC__
localeconv(void)
#else
localeconv()
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	static char *curloc = (char *)_str_c;
	static struct lconv *cur = (struct lconv *)&locale_c;
	struct stat stbuf;
	struct lconv *p;
	char *loc;
	int fd;

	STDLOCK(&lock);
	/*
	* Handle the LC_MONETARY portion.  If the locale differs from the
	* last time, attempt to read and adjust the locale's LC_MONETARY
	* information, falling back on the "C" locale's if anything fails.
	*/
	if ((loc = _locale[LC_MONETARY]) != curloc)
	{
		if (loc == _str_c)
		{
		err0:;
			p = (struct lconv *)&locale_c;
		}
		else if ((fd = _openlocale(LC_MONETARY, loc, (char *)0)) == -1)
		{
		err1:;
			loc = (char *)_str_c;
			goto err0;
		}
		else if (fstat(fd, &stbuf) != 0
			|| stbuf.st_size < sizeof(struct lconv)
			|| (p = (struct lconv *)malloc(stbuf.st_size + 2 + 2))
				== 0)
		{
		err2:;
			close(fd);
			goto err1;
		}
		else if (read(fd, (void *)p, stbuf.st_size) != stbuf.st_size)
		{
			free((void *)p);
			goto err2;
		}
		else
		{
			close(fd);
			p->decimal_point = 0 + stbuf.st_size + (char *)p;
			p->thousands_sep = 2 + stbuf.st_size + (char *)p;
			p->int_curr_symbol = (int)p->int_curr_symbol
				+ (char *)&p[1];
			p->currency_symbol = (int)p->currency_symbol
				+ (char *)&p[1];
			p->mon_decimal_point = (int)p->mon_decimal_point
				+ (char *)&p[1];
			p->mon_thousands_sep = (int)p->mon_thousands_sep
				+ (char *)&p[1];
			p->mon_grouping = (int)p->mon_grouping
				+ (char *)&p[1];
			p->positive_sign = (int)p->positive_sign
				+ (char *)&p[1];
			p->negative_sign = (int)p->negative_sign
				+ (char *)&p[1];
		}
		curloc = loc;
		if (cur != &locale_c)
			free((void *)cur);
		cur = p;
	}
	/*
	* Fill in the LC_NUMERIC portion.  Attempt to assign the three strings
	* unless we're in the "C" locale and cur is already there, or if we
	* can't allocate a copy when LC_MONETARY is "C", but LC_NUMERIC isn't.
	*/
	if (cur == &locale_c)
	{
		if (_locale[LC_NUMERIC] == _str_c
			|| (p = (struct lconv *)malloc(
				sizeof(struct lconv) + 2 + 2)) == 0)
		{
			goto skip;
		}
		*p = locale_c;
		p->decimal_point = 0 + (char *)&p[1];
		p->thousands_sep = 2 + (char *)&p[1];
		cur = p;
	}
	/*
	* cur must be pointing at an allocated struct lconv.  Note that space
	* for two additional length one strings is always allocated as well.
	* This is the space pointed to by decimal_point and thousands_sep.
	*/
	cur->decimal_point[1] = '\0';
	cur->decimal_point[0] = _numeric[0];
	cur->thousands_sep[1] = '\0';
	cur->thousands_sep[0] = _numeric[1];
	cur->grouping = (char *)_grouping;
skip:;
	STDUNLOCK(&lock);
	return cur;
}

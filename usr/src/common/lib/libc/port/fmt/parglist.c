/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:fmt/parglist.c	1.7"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#ifdef __STDC__
#   include <stdarg.h>
#else
#   include <varargs.h>
#endif
#include <ctype.h>
#include <string.h>
#include <math.h>
#include "stdiom.h"
#include "format.h"

#ifdef WIDE
#   include "wcharm.h"
#   define FCNNAME	_wparglist
#   define CHAR		wchar_t
#   define STRCHR	wcschr
#   define ISDIGIT(c)	('0' <= (c) && (c) <= '9')
#   define ATOI(s)	wcstol(s, (wchar_t **)0, 10)
#else
#   define FCNNAME	_parglist
#   define CHAR		char
#   define STRCHR	strchr
#   define ISDIGIT(c)	isdigit(c)
#   define ATOI(s)	atoi(s)
#endif

enum	/* all the various types that can be given to va_arg() */
{
	T_INT,	/* default */
	T_LONG,
#ifndef NO_LONG_LONG_EMULATE
	T_LLONG,
#endif
#if defined(WIDE) || !defined(NO_MSE)
	T_WINT,
#endif
	T_DBL,
	T_LDBL,
	T_VPTR,
	T_CPTR,
	T_SPTR,
	T_IPTR,
	T_LPTR,
#if defined(WIDE) || !defined(NO_MSE)
	T_WPTR,
#endif
	T_FPTR,
	T_DPTR,
	T_LDPTR,
	T_PPTR
};

static void
#ifdef __STDC__
adv(RA_va_list *p, int type)	/* advance "p->ap" by specified type */
#else
adv(p, type)RA_va_list *p; int type;
#endif
{
	switch (type)
	{
#if defined(WIDE) || !defined(NO_MSE)
	case T_WPTR:
		(void)va_arg(p->ap, wchar_t *);
		break;
	case T_WINT:
		/*CONSTANTCONDITION*/
		if (sizeof(wint_t) >= sizeof(int))
		{
			(void)va_arg(p->ap, wint_t);
			break;
		}
		/*FALLTHROUGH*/
#endif
	default:
		(void)va_arg(p->ap, int);
		break;
#ifndef NO_LONG_LONG_EMULATE
	case T_LLONG:
		(void)va_arg(p->ap, long);
		/*FALLTHROUGH*/
#endif
	case T_LONG:
		(void)va_arg(p->ap, long);
		break;
	case T_DBL:
		(void)va_arg(p->ap, double);
		break;
#ifndef NO_LONG_DOUBLE
	case T_LDBL:
		(void)va_arg(p->ap, long double);
		break;
#endif
	case T_VPTR:
		(void)va_arg(p->ap, void *);
		break;
	case T_CPTR:
		(void)va_arg(p->ap, char *);
		break;
	case T_SPTR:
		(void)va_arg(p->ap, short *);
		break;
	case T_IPTR:
		(void)va_arg(p->ap, int *);
		break;
	case T_LPTR:
		(void)va_arg(p->ap, long *);
		break;
	case T_FPTR:
		(void)va_arg(p->ap, float *);
		break;
	case T_DPTR:
		(void)va_arg(p->ap, double *);
		break;
#ifndef NO_LONG_DOUBLE
	case T_LDPTR:
		(void)va_arg(p->ap, long double *);
		break;
#endif
	case T_PPTR:
		(void)va_arg(p->ap, void **);
		break;
	}
}

	/*
	* Manage positional argument list conversion specifications.
	*
	* Invoke either with printf-family format (scan == 0) or
	* with scanf-family format (scan != 0).
	*
	* Invoked with (pos <= len) to fill an array with up to "len"
	* RA_va_list's with cached "ap" locations.  In this case, "lp"
	* points to the first element of the array to fill.  The first
	* element must contain the original "ap" value for the ... args.
	*
	* Invoked with (pos > len) to change "*lp" to be ready for a
	* va_arg() invocation for the "pos"-th argument.  "lp" must
	* hold the value of the "len"-th element of the previously
	* filled-in array.
	*/
void
#ifdef __STDC__
FCNNAME(const CHAR *fmt, RA_va_list *lp, int len, int pos, int scan)
#else
FCNNAME(fmt,lp,len,pos,scan)const CHAR *fmt; RA_va_list *lp; int len,pos,scan;
#endif
{
	RA_va_list tmp;
	int high, maxpos, argno;

	tmp = *lp;
	if (pos > len)		/* position "*lp" for arg "pos" */
	{
		high = len;
	}
	else if (len > 0)	/* first time: fill array with "int" */
	{
		register RA_va_list *p = &lp[len];
		register size_t n = len;

		do
			(--p)->flag = T_INT;
		while (--n != 0);
		high = 0;
		maxpos = 0;
	}
	argno = 1;	/* default to initial argument */
	/*
	* Outer loop only taken when positioning "*lp" for arg "pos".
	*/
	for (;;)
	{
		register const CHAR *s = fmt;
		register int n, type;
		const CHAR *x;

		/*
		* This code assumes that a byte with value % cannot be part
		* of a multibyte character.
		*/
		while ((s = STRCHR(s, '%')) != 0) /* each conversion spec. */
		{
			x = s;
			while (++s, ISDIGIT(*s))
				;
			if (*s == '$')	/* move to specified argument */
			{
				if ((n = ATOI(x + 1)) <= 0) /* bad position */
					n = 1;
				argno = n;
			}
			n = '\0';
		again:;
			switch (*s++)
			{
			default:	/* no argument (e.g. %%) */
				continue;
			case 'l':
				if (n == 'l')
				{
					n = 'L';
					goto again;
				}
				/*FALLTHROUGH*/
			case 'h':
			case 'L':
				n = s[-1];
				/*FALLTHROUGH*/
			case '.':
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
			case '-':
			case '+':
			case '#':
			case '\'':
			case ' ':
			case '$':
				goto again;
			case '*':
				if (scan)	/* no argument */
					continue;
				if (ISDIGIT(*s))
				{
					/*
					* Positional width or precision.
					* Always has "int" type:  Default
					* behavior does the right stuff.
					*/
					if (high == 0 && (n = ATOI(s)) > 0
						&& n <= len && n > maxpos)
					{
						maxpos = n;
					}
					while (++s, ISDIGIT(*s))
						;
					goto again; /* *s should be '$' */
				}
				n = '*';
				type = T_INT;
				break;
			case 'c':
#if defined(WIDE) || !defined(NO_MSE)
				if (n == 'l')
				{
			case 'C':
					if (scan)
						type = T_WPTR;
					else
						type = T_WINT;
				}
				else
#endif
				if (scan)
					type = T_CPTR;
				else
					type = T_INT;
				break;
			case 's':
#if defined(WIDE) || !defined(NO_MSE)
				if (n == 'l')
				{
			case 'S':
					type = T_WPTR;
				}
				else
#endif
					type = T_CPTR;
				break;
			case '[':
				if (!scan)
					continue;	/* bogus spec. */
				if (*s == '^')
					s++;
				while (*s != '\0' && *++s != ']')
					;
#ifndef NO_MSE
				if (n == 'l')
					type = T_WPTR;
				else
#endif
					type = T_CPTR;
				break;
#ifndef NO_CI4
			case 'B':
			case 'D':
			case 'I':
			case 'O':
			case 'U':
				if (_lib_version != c_issue_4 || !scan)
					continue;	/* bogus spec. */
				type = T_LPTR;
				break;
#endif
			case 'X':
#ifndef NO_CI4
				if (scan && _lib_version == c_issue_4
					&& n == '\0')
				{
					type = T_LPTR;
					break;
				}
				/*FALLTHROUGH*/
#endif
			case 'b':
			case 'd':
			case 'i':
			case 'o':
			case 'u':
			case 'x':
				if (scan)
				{
					if (n == 'l')
						type = T_LPTR;
					else if (n == 'h')
						type = T_SPTR;
#ifndef NO_LONG_LONG_EMULATE
					else if (n == 'L')
						type = T_LPTR;
#endif
					else
						type = T_IPTR;
				}
				else if (n == 'l')
					type = T_LONG;
#ifndef NO_LONG_LONG_EMULATE
				else if (n == 'L')
					type = T_LLONG;
#endif
				else
					type = T_INT;
				break;
			case 'p':
				if (scan)
					type = T_PPTR;
				else
					type = T_VPTR;
				break;
#ifndef NO_NCEG_FPE
			case 'A':
#endif
			case 'E':
			case 'F':
			case 'G':
#ifndef NO_CI4
				if (scan && _lib_version == c_issue_4
					&& n == '\0')
				{
					type = T_DPTR;
					break;
				}
				/*FALLTHROUGH*/
#endif
#ifndef NO_NCEG_FPE
			case 'a':
#endif
			case 'e':
			case 'f':
			case 'g':
				if (scan)
				{
					if (n == '\0')
						type = T_FPTR;
#ifndef NO_LONG_DOUBLE
					else if (n == 'L')
						type = T_LDPTR;
#endif
					else
						type = T_DPTR;
				}
#ifndef NO_LONG_DOUBLE
				else if (n == 'L')
					type = T_LDBL;
#endif
				else
					type = T_DBL;
				break;
			case 'n':
				if (n == 'l')
					type = T_LPTR;
				else if (n == 'h')
					type = T_SPTR;
				else
					type = T_IPTR;
				break;
			}
			if (high == 0)	/* filling array mode */
			{
				if (argno <= len)
				{
					lp[argno - 1].flag = type;
					if (argno > maxpos)
						maxpos = argno;
				}
			}
			else if (argno == high)
			{
				adv(&tmp, type);
				if (++high >= pos)	/* reached target */
				{
					*lp = tmp;
					return;
				}
			}
			argno++;	/* default to next */
			if (n == '*')
			{
				n = '\0';
				goto again;
			}
		}
		/*
		* Scanned entire format string.
		* If array filling, do so (below) and return.
		* Otherwise, take "high" as an "int" and restart scan.
		*/
		if (high == 0)
			break;		/* go fill array */
		adv(&tmp, T_INT);
		if (++high >= pos)	/* reached target */
		{
			*lp = tmp;
			return;
		}
	}
	if (maxpos != 0) /* fill array, overwriting flag with ap value */
	{
		register RA_va_list *p = lp;
		register int type, n = maxpos;

		do
		{
			type = p->flag;
			*p++ = tmp;
			adv(&tmp, type);
		} while (--n != 0);
	}
}

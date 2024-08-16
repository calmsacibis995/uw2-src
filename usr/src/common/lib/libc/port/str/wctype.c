/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wctype.c	1.7"

#include "synonyms.h"
#include "wcharm.h"
#include "_locale.h"
#include "stdlock.h"

wctype_t
#ifdef __STDC__
wctype(const char *name)
#else
wctype(name)const char *name;
#endif
{
	struct lc_ctype *lcp;
	wctype_t ret;

	/*
	* Search predefined table first.
	*/
	{
		static const struct t
		{ 
			char		name[7];
			wctype_t	bits;
		} table[] =
		{
			"alnum",	_PD_ALNUM,
			"alpha",	_PD_ALPHA,
			"blank",	_PD_BLANK,
			"cntrl",	_PD_CNTRL,
			"digit",	_PD_DIGIT,
			"graph",	_PD_GRAPH,
			"lower",	_PD_LOWER,
			"print",	_PD_PRINT,
			"punct",	_PD_PUNCT,
			"space",	_PD_SPACE,
			"upper",	_PD_UPPER,
			"xdigit",	_PD_XDIGIT,
		};
		register const struct t *p = table;
		register int ch = *name;

		do
		{
			if (ch < p->name[0])
				break;
			if (ch > p->name[0])
				continue;
			if (strcmp(&name[1], &p->name[1]) == 0)
				return p->bits;
		} while (++p < &table[sizeof(table) / sizeof(table[0])]);
	}
	/*
	* Then search locale-defined table.
	*/
	ret = 0;
	if ((lcp = _lc_ctype()) != 0)
	{
		register const struct t_ctype *tp;
		register const char *base;
		register size_t ntp;

		if ((tp = lcp->typetab) != 0
			&& (base = (char *)lcp->strtab) != 0)
		{
			ntp = lcp->ntypetab;
			do
			{
				if (strcmp(name, &base[tp->name]) == 0)
				{
					if ((ret = tp->type) == 0)
					{
						ret = (tp - lcp->typetab);
						ret <<= 8;
						ret |= XWCTYPE;
					}
					break;
				}
			} while (++tp, --ntp != 0);
		}
		STDUNLOCK(lcp->lockptr);
	}
	return ret;
}

int
#ifdef __STDC__
iswctype(wint_t wi, wctype_t x)
#else
iswctype(wi, x)wint_t wi; wctype_t x;
#endif
{
	if ((wi & ~(wchar_t)0xff) == 0)
	{
		switch (x)
		{
		case _PD_BLANK:
			if (wi == ' ' || wi == '\t')
				return _B;
			/*FALLTHROUGH*/
		case _PD_ALNUM:
		case _PD_ALPHA:
		case _PD_CNTRL:
		case _PD_DIGIT:
		case _PD_GRAPH:
		case _PD_LOWER:
		case _PD_PRINT:
		case _PD_PUNCT:
		case _PD_SPACE:
		case _PD_UPPER:
		case _PD_XDIGIT:
			return x & (_ctype + 1)[wi];
		}
	}
	return __iswctype(wi, x);
}

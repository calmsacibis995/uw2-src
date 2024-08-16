/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcswidth.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <ctype.h>
#include "wcharm.h"
#include "stdlock.h"

#ifdef __STDC__
	#pragma weak wcswidth = _wcswidth
#endif

int
#ifdef __STDC__
wcswidth(const wchar_t *s, size_t n)
#else
wcswidth(s, n)const wchar_t *s; size_t n;
#endif
{
	struct t_wctype dummy;
	register const struct t_wctype *wcp;
	register const unsigned char *ucp;
	register wctype_t *tp;
	struct lc_ctype *lcp;
	wchar_t wc;
	int len;

	if (n == 0)
		return 0;
	len = 0;
	wc = *s;
	if (!multibyte || (lcp = _lc_ctype()) == 0)
	{
		/*
		* Each wide character must be a regular printable byte.
		*/
		do
		{
			if ((wc & ~(wchar_t)0xff) != 0)
				return -1;
			if (!isprint(wc))
			{
				if (wc == '\0')
					return len;
				return -1;
			}
			len++;
		} while (wc = *++s, --n != 0);
		return len;
	}
	/*
	* Local t_wctype structure allows caching of the most recent
	* "region" of the extended character set.  The range in dummy
	* cannot match the first wide character.
	*/
	dummy.tmin = dummy.tmax = 0;
	wcp = &dummy;
	do
	{
		if ((wc & ~(wchar_t)0x7f) == 0)
		{
			if (!isprint(wc))
			{
				if (wc != '\0')
					len = -1;
				break;
			}
			len++;
		}
		else
		{
			if (wc < wcp->tmin || wcp->tmax < wc)
			{
				/*
				* This wide character falls in a different
				* region of the extended character set.
				*/
				if ((wcp = _t_wctype(lcp, wc)) == 0
					|| wcp->type == 0)
				{
					len = -1;
					break;
				}
				ucp = &lcp->base[wcp->index];
				tp = (wctype_t *)&lcp->base[wcp->type];
			}
			wc -= wcp->tmin;
			if ((tp[ucp[wc]] & _PD_PRINT) == 0)
			{
				len = -1;
				break;
			}
			if (wcp->dispw > 255)
				len += lcp->base[wcp->dispw + wc];
			else
				len += wcp->dispw;
		}
	} while (wc = *++s, --n != 0);
	STDUNLOCK(lcp->lockptr);
	return len;
}

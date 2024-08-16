/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wctomb.c	1.16"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdlib.h>
#include <limits.h>
#include <errno.h>
#include "wcharm.h"

#ifndef NO_MSE

#ifndef NO_IGNORE_MBSTATE
	#pragma weak wcrtomb = wctomb
#else
	#pragma weak wcrtomb = _wcrtomb

/*ARGSUSED2*/
int
#ifdef __STDC__
_wcrtomb(char *s, wchar_t wc, mbstate_t *ps)
#else
_wcrtomb(s, wc, ps)char *s; wchar_t wc; mbstate_t *ps;
#endif
{
	return wctomb(s, wc);
}

#endif /*NO_IGNORE_MBSTATE*/

#ifdef __STDC__
	#pragma weak wctob = _wctob
#endif

int
#ifdef __STDC__
wctob(wint_t wi)
#else
wctob(wi)wint_t wi;
#endif
{
	if ((wi & ~(wchar_t)0xff) == 0)
		return wi;
	if ((wi & EUCMASK) == P11 && eucw1 == 1)
	{
		if (((wi &= ~EUCMASK) & ~(wchar_t)0x7f) == 0
			&& (wi |= 0x80) >= 0xa0)
		{
			return wi;
		}
	}
	return -1;	/* == EOF */
}

#endif /*NO_MSE*/

int
#ifdef __STDC__
wctomb(char *s, register wchar_t wc)
#else
wctomb(s, wc)char *s; register wchar_t wc;
#endif
{
	register unsigned char *us;
	register int n, len, cs1;

	if ((us = (unsigned char *)s) == 0)
		return 0;	/* stateless encoding */
	switch ((wc >> EUCDOWN) & DOWNMSK)
	{
	default:
	bad:;
		errno = EILSEQ;
		return -1;
	case DOWNP00:
		if ((wc & ~(wchar_t)0xff) != 0
			|| wc >= 0xa0 && multibyte && eucw1 != 0)
		{
			goto bad;
		}
		*us = wc;
		return 1;
	case DOWNP11:
		n = eucw1;
		cs1 = 1;
		break;
	case DOWNP01:
		n = eucw2;
		*us++ = SS2;
		cs1 = 0;
		break;
	case DOWNP10:
		n = eucw3;
		*us++ = SS3;
		cs1 = 0;
		break;
	}
	switch (len = n)	/* fill in backwards */
	{
	case 0:
		goto bad;
#if MB_LEN_MAX > 5
	default:
		us += n;
		do
		{
#if UCHAR_MAX == 0xff
			*--us = wc | 0x80;
#else
			*--us = (wc | 0x80) & 0xff;
#endif
			wc >>= 7;
		} while (--n != 0);
		break;
#endif /*MB_LEN_MAX > 5*/
	case 4:
#if UCHAR_MAX == 0xff
		us[3] = wc | 0x80;
#else
		us[3] = (wc | 0x80) & 0xff;
#endif
		wc >>= 7;
		/*FALLTHROUGH*/
	case 3:
#if UCHAR_MAX == 0xff
		us[2] = wc | 0x80;
#else
		us[2] = (wc | 0x80) & 0xff;
#endif
		wc >>= 7;
		/*FALLTHROUGH*/
	case 2:
#if UCHAR_MAX == 0xff
		us[1] = wc | 0x80;
#else
		us[1] = (wc | 0x80) & 0xff;
#endif
		wc >>= 7;
		/*FALLTHROUGH*/
	case 1:
#if UCHAR_MAX == 0xff
		us[0] = wc | 0x80;
#else
		us[0] = (wc | 0x80) & 0xff;
#endif
		break;
	}
	if (!cs1)
		len++;
	else if (*us < 0xa0)	/* C1 cannot be first byte */
		goto bad;
	return len;
}

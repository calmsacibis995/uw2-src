/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/mbtowc.c	1.14"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdlib.h>
#include <errno.h>
#include "wcharm.h"

#ifndef NO_MSE

#ifndef NO_IGNORE_MBSTATE
	#pragma weak mbrlen = mblen
	#pragma weak mbrtowc = mbtowc
#else
	#pragma weak mbrlen = _mbrlen
	#pragma weak mbrtowc = _mbrtowc

#undef smblen

/*ARGSUSED2*/
int
#ifdef __STDC__
_mbrlen(const char *s, size_t n, mbstate_t *ps)
#else
_mbrlen(s, n, ps)const char *s; size_t n; mbstate_t *ps;
#endif
{
	return mbtowc((wchar_t *)0, s, n);
}

/*ARGSUSED3*/
int
#ifdef __STDC__
_mbrtowc(wchar_t *p, const char *s, size_t n, mbstate_t *ps)
#else
_mbrtowc(p, s, n, ps)wchar_t *p; const char *s; size_t n; mbstate_t *ps;
#endif
{
	return mbtowc(p, s, n);
}

#endif /*NO_IGNORE_MBSTATE*/

#endif /*NO_MSE*/

#ifdef __STDC__
	#pragma weak btowc = _btowc
#endif

wint_t
#ifdef __STDC__
btowc(int ch)
#else
btowc(ch)int ch;
#endif
{
	if ((ch & ~0xff) != 0)
		ch = -1; /* -1 == [W]EOF */
	else if (ch >= 0x80 && multibyte)
	{
		if (ch < 0xa0)
		{
			if (ch == SS2)
			{
				if (eucw2 != 0)
					ch = -1;
			}
			else if (ch == SS3)
			{
				if (eucw3 != 0)
					ch = -1;
			}
		}
		else if (eucw1 != 0)
		{
			if (eucw1 == 1)
				return (ch & 0x7f) | (wint_t)P11;
			ch = -1;
		}
	}
	return ch;
}

#undef mblen

int
#ifdef __STDC__
mblen(const char *s, size_t n)
#else
mblen(s, n)const char *s; size_t n;
#endif
{
	return mbtowc((wchar_t *)0, s, n);
}

int
#ifdef __STDC__
mbtowc(wchar_t *p, const char *s, size_t n)
#else
mbtowc(p, s, n)wchar_t *p; const char *s; size_t n;
#endif
{
	register int ch;
	register const unsigned char *us;
	register int len;
	register wchar_t wc;
	int retval;
	wchar_t mask;

	if ((us = (const unsigned char *)s) == 0)
		return 0;	/* stateless encoding */
	if (n == 0)
		return -1;	/* don't set errno for too few bytes */
	if ((ch = *us) < 0x80 || !multibyte)
	{
	onebyte:;
		if (p != 0)
			*p = ch;
		return ch != '\0';
	}
	if (ch < 0xa0)	/* C1 (i.e., metacontrol) byte */
	{
		if (ch == SS2)
		{
			if ((len = eucw2) == 0)
				goto onebyte;
			mask = P01;
		}
		else if (ch == SS3)
		{
			if ((len = eucw3) == 0)
				goto onebyte;
			mask = P10;
		}
		else
			goto onebyte;
		wc = 0;
		retval = len + 1;
	}
	else
	{
		if ((len = eucw1) == 0)
			goto onebyte;
		mask = P11;
		wc = ch & 0x7f;
		retval = len;
		if (--len == 0)
			goto out;
	}
	if (retval > n)
		return -1;	/* don't set errno for too few bytes */
	do
	{
		if ((ch = *++us) < 0x80)
		{
			errno = EILSEQ;
			return -1;
		}
		wc <<= 7;
		wc |= ch & 0x7f;
	} while (--len != 0);
out:;
	if (p != 0)
		*p = wc | mask;
	return retval;
}

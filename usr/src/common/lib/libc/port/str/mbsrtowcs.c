/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/mbsrtowcs.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <errno.h>
#include <string.h>
#include "wcharm.h"

#ifndef NO_MSE

#ifndef NO_IGNORE_MBSTATE
	#pragma weak mbsrtowcs = _xmbstowcs
#else
	#pragma weak mbsrtowcs = _mbsrtowcs

/*ARGSUSED3*/
int
#ifdef __STDC__
_mbsrtowcs(wchar_t *d, const char **s, size_t n, mbstate_t *ps)
#else
_mbsrtowcs(d, s, n, ps)wchar_t *d; const char **s; size_t n; mbstate_t *ps;
#endif
{
	return _xmbstowcs(d, s, n);
}

#endif /*NO_IGNORE_MBSTATE*/

#ifdef __STDC__
	#pragma weak mbsinit = _mbsinit
#endif

/*ARGSUSED*/
int
#ifdef __STDC__
mbsinit(const mbstate_t *ps)
#else
mbsinit(ps)const mbstate_t *ps;
#endif
{
	return 1;
}

#endif /*NO_MSE*/

	/*
	* Restartable multibyte string to wide string conversion.
	* Returns:
	*   (size_t)-1	Initial multibyte character is invalid.
	*   *		Otherwise, the number of codes written,
	*		which can be zero if there's no more room.
	* On return, *src is updated:
	*   0	If the end of the input multibyte string is reached.
	*   *	Otherwise, the next multibyte character to process,
	*	or the bad multibyte character for a (size_t)-1 return.
	*/

size_t
#ifdef __STDC__
_xmbstowcs(wchar_t *dst, const char **src, size_t len)
#else
_xmbstowcs(dst, src, len)wchar_t *dst; const char **src; size_t len;
#endif
{
	register const unsigned char *sp;
	register wchar_t wc;
	register int n, ch;
	wchar_t mask;
	size_t origlen;

	sp = *(const unsigned char **)src;
	if (!multibyte)		/* just moving bytes */
	{
		if (dst == 0)	/* just string length */
		{
			origlen = strlen((const char *)sp);
			*src = 0;
		}
		else
		{
			origlen = len;
			for (wc = *sp;; wc = *++sp)
			{
				if (wc == '\0')
				{
					origlen -= len;
					*src = 0;
					break;
				}
				if (len == 0)
				{
					*src = (const char *)sp;
					break;
				}
				*dst++ = wc;
				len--;
			}
		}
		return origlen;
	}
	if (dst == 0)
		len = ~(size_t)0;	/* unlimited */
	origlen = len;
	for (wc = *sp;; wc = *++sp)
	{
		/*
		* The check for enough space is delayed until after the
		* check for a one byte character.  This covers the special
		* case of the end of the input string.
		*/
		if (wc < 0x80)	/* fits in 7 bits: ASCII */
		{
			if (wc == '\0')	/* reached end of input string */
			{
				*src = 0;
				return origlen - len;
			}
		onebyte:;
			if (len == 0)
			{
				*src = (const char *)sp;
				return origlen;
			}
			len--;
			if (dst != 0)
				*dst++ = wc;
			continue;
		}
		if (wc < 0xa0)	/* C1 (i.e., metacontrol) byte */
		{
			if (wc == SS2)
			{
				if ((n = eucw2) == 0)
					goto onebyte;
				mask = P01;
			}
			else if (wc == SS3)
			{
				if ((n = eucw3) == 0)
					goto onebyte;
				mask = P10;
			}
			else
				goto onebyte;
			wc = 0;
		}
		else
		{
			if ((n = eucw1) == 0)
				goto onebyte;
			wc &= 0x7f;
			if (--n == 0)
			{
				wc |= P11;
				goto onebyte;
			}
			mask = P11;
		}
		if (len == 0)	/* don't bother checking the rest now */
		{
			*src = (const char *)sp;
			return origlen;
		}
		do
		{
			if ((ch = *++sp) < 0x80)
			{
				errno = EILSEQ;
				if (mask == P11)
					n = eucw1 - n;
				else if (mask == P01)
					n = 1 + eucw2 - n;
				else /* mask == P10 */
					n = 1 + eucw3 - n;
				sp -= n;
				if (*src == (const char *)sp)
					return (size_t)-1;
				*src = (const char *)sp;
				return origlen - len;
			}
			wc <<= 7;
			wc |= ch & 0x7f;
		} while (--n != 0);
		len--;
		if (dst != 0)
			*dst++ = wc | mask;
	}
}

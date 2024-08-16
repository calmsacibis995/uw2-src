/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcsrtombs.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <errno.h>
#include <limits.h>
#include "wcharm.h"

#ifndef NO_MSE

#ifndef NO_IGNORE_MBSTATE
	#pragma weak wcsrtombs = _xwcstombs
#else
	#pragma weak wcsrtombs = _wcsrtombs

/*ARGSUSED3*/
int
#ifdef __STDC__
_wcsrtombs(char *d, const wchar_t **s, size_t n, mbstate_t *ps)
#else
_wcsrtombs(d, s, n, ps)char *d; const wchar_t **s; size_t n; mbstate_t *ps;
#endif
{
	return _xwcstombs(d, s, n);
}

#endif /*NO_IGNORE_MBSTATE*/

#endif /*NO_MSE*/

size_t
#ifdef __STDC__
_iwcstombs(char *d, const wchar_t **s, size_t n)
#else
_iwcstombs(d, s, n)char *d; const wchar_t **s; size_t n;
#endif
{
	size_t sz, total = 0;

	while ((sz = _xwcstombs(d, s, n)) != 0)
	{
		if (sz == (size_t)-1)	/* first code is bad */
		{
			++*s;	/* skip it */
			continue;
		}
		total += sz;
		if (*s == 0 || errno == 0)
			break;
		d += sz;
		n -= sz;
	}
	return total;
}

	/*
	* Restartable wide string to multibyte string conversion.
	* Returns:
	*   (size_t)-1	Initial wide character is invalid.
	*   *		Otherwise, the number of bytes written,
	*		which can be zero if not enough room.
	* On return, *src is updated:
	*   0	If the end of the input wide string is reached.
	*   *	Otherwise, the next wide character to process,
	*	or the bad code for a (size_t)-1 return.
	*/

size_t
#ifdef __STDC__
_xwcstombs(char *dst, const wchar_t **src, size_t len)
#else
_xwcstombs(dst, src, len)char *dst; const wchar_t **src; size_t len;
#endif
{
	register unsigned char *dp;
	register wchar_t wc;
	register int n, cs1;
	register const wchar_t *sp;
	size_t origlen;
	unsigned char buf[MB_LEN_MAX];

	if ((dp = (unsigned char *)dst) == 0)
	{
		len = ~(size_t)0;	/* unlimited */
		dp = &buf[0];		/* place to drop bytes */
	}
	cs1 = 0;
	origlen = len;
	sp = *src;
	for (wc = *sp;; wc = *++sp)
	{
		switch ((wc >> EUCDOWN) & DOWNMSK)
		{
		default:
		bad:;
			errno = EILSEQ;
			if (*src == sp)
				return (size_t)-1;
			*src = sp;
			return origlen - len;
		case DOWNP00:
			if ((wc & ~(wchar_t)0xff) != 0
				|| wc >= 0xa0 && multibyte && eucw1 != 0)
			{
				goto bad;
			}
			if (wc == 0)	/* reached end-of-string */
			{
				*src = 0;
				return origlen - len;
			}
			if (len == 0)
			{
			tooshort:;
				*src = sp;
				return origlen - len;
			}
			if (dst != 0)
				*dp++ = wc;
			len--;
			continue;
		case DOWNP11:
			if ((n = eucw1) > len)
				goto tooshort;
			cs1 = 1;
			break;
		case DOWNP01:
			if ((n = eucw2) >= len)
				goto tooshort;
			*dp++ = SS2;
			break;
		case DOWNP10:
			if ((n = eucw3) >= len)
				goto tooshort;
			*dp++ = SS3;
			break;
		}
		switch (n)	/* fill in backwards */
		{
#if MB_LEN_MAX > 5
			int i;

		default:
			i = n;
			dp += n;
			do
			{
#if UCHAR_MAX == 0xff
				*--dp = wc | 0x80;
#else
				*--dp = (wc | 0x80) & 0xff;
#endif
				wc >>= 7;
			} while (--n != 0);
			n = i;
			break;
#endif /*MB_LEN_MAX > 5*/
		case 0:
			goto bad;
		case 4:
#if UCHAR_MAX == 0xff
			dp[3] = wc | 0x80;
#else
			dp[3] = (wc | 0x80) & 0xff;
#endif
			wc >>= 7;
			/*FALLTHROUGH*/
		case 3:
#if UCHAR_MAX == 0xff
			dp[2] = wc | 0x80;
#else
			dp[2] = (wc | 0x80) & 0xff;
#endif
			wc >>= 7;
			/*FALLTHROUGH*/
		case 2:
#if UCHAR_MAX == 0xff
			dp[1] = wc | 0x80;
#else
			dp[1] = (wc | 0x80) & 0xff;
#endif
			wc >>= 7;
			/*FALLTHROUGH*/
		case 1:
#if UCHAR_MAX == 0xff
			dp[0] = wc | 0x80;
#else
			dp[0] = (wc | 0x80) & 0xff;
#endif
			break;
		}
		if (!cs1)
		{
			if (dst == 0)
				dp = &buf[0];
			else
				dp += n;
			n++;
		}
		else
		{
			cs1 = 0;
			if (*dp < 0xa0)	/* C1 cannot be first byte */
				goto bad;
			if (dst != 0)
				dp += n;
		}
		len -= n;
	}
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/wcsftime.c	1.5"

#include "synonyms.h"
#include <stdlib.h>
#include <limits.h>
#include <time.h>
#include "wcharm.h"

#ifdef __STDC__
	#pragma weak wcsftime = _wcsftime
#endif

size_t
#ifdef __STDC__
wcsftime(wchar_t *dst, size_t max, const wchar_t *fmt, const struct tm *ptm)
#else
wcsftime(dst,max,fmt,ptm)wchar_t*dst;size_t max;const wchar_t*fmt;const struct tm*ptm;
#endif
{
	char rstr[MB_LEN_MAX * 250], fstr[5];
	const char *p;
	wchar_t *wp;
	size_t n;

	fstr[0] = 'x';	/* forces nonzero strftime() return */
	fstr[1] = '%';
	wp = dst;
	while (*fmt != '\0')
	{
		if (*fmt != '%')
		{
		copy:;
			if (max == 0)	/* not enough room */
			{
				wp = dst;
				break;
			}
			*wp++ = *fmt++;
			max--;
			continue;
		}
		/*
		* Put together the byte string version of the specifier
		* so that strftime() can do the real work.
		*/
		if (fmt[1] > UCHAR_MAX)
			goto copy;
		fstr[2] = fmt[1];
		if (fmt[1] != 'E' && fmt[1] != 'O')
		{
			fstr[3] = '\0';
			fmt += 2;
		}
		else if (fmt[2] > UCHAR_MAX)
			goto copy;
		else
		{
			fstr[3] = fmt[2];
			fstr[4] = '\0';
			fmt += 3;
		}
		/*
		* For now, assume that the result of any single format
		* is small enough to fit in rstr[], and thus a 0 return
		* from strftime() is an indication of some other error.
		*/
		if ((n = strftime(rstr, sizeof(rstr), fstr, ptm)) == 0)
		{
			wp = dst;
			break;
		}
		/*
		* Convert byte string to wide characters.  No attempt
		* is made to recover from bad multibyte characters.
		*/
		p = &rstr[1];		/* skip initial 'x' */
		n = _xmbstowcs(wp, &p, max);
		if (p != 0)		/* didn't convert entire string */
		{
			wp = dst;
			break;
		}
		wp += n;
		max -= n;
	}
	if (max == 0)	/* not enough room for termination */
		wp = dst;
	*wp = '\0';
	return wp - dst;
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/_mb2wc.c	1.2"

#include "synonyms.h"
#include "wcharm.h"

	/*
	* Complete the wide character whose first byte is in *wp,
	* from the rest of the bytes pointed to by ps.
	* This function assumes that it is called only when
	* ISONEBYTE(*wp) was false.  [*wp >= 0x80 && multibyte]
	* Returns a negative value on failure; otherwise the
	* number of additional bytes consumed.
	*/
int
#ifdef __STDC__
_mb2wc(wint_t *wp, const unsigned char *ps)
#else
_mb2wc(wp, ps)wint_t *wp; const unsigned char *ps;
#endif
{
	wint_t wc;
	int ret, len;

	if (utf8)
	{
		/*
		* 110xxxxx 10xxxxxx
		* 1110xxxx 10xxxxxx 10xxxxxx
		* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
		* 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		* 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
		*/
		if ((wc = *wp) < 0xf0)
		{
			if (wc < 0xe0)
			{
				wc &= 0x1f;
				ret = 1;
				goto need1;
			}
#ifdef UTFERR
			else if (wc < 0xc0)
				return -1;
#endif
			wc &= 0x0f;
			ps += 1;
			ret = 2;
			goto need2;
		}
		else if (wc < 0xf8)
		{
			wc &= 0x07;
			ps += 2;
			ret = 3;
			goto need3;
		}
		else if (wc < 0xfc)
		{
			wc &= 0x03;
			ps += 3;
			ret = 4;
			goto need4;
		}
#ifdef UTFERR
		else if (wc >= 0xfe)
			return -1;
#endif
		wc &= 0x01;
		ps += 4;
		ret = 5;
#ifdef UTFERR
		if ((ps[-4] & 0xc0) != 0x80)
			return -1;
#endif
		wc <<= 6;
		wc |= ps[-4] & 0x3f;
	need4:;
#ifdef UTFERR
		if ((ps[-3] & 0xc0) != 0x80)
			return -1;
#endif
		wc <<= 6;
		wc |= ps[-3] & 0x3f;
	need3:;
#ifdef UTFERR
		if ((ps[-2] & 0xc0) != 0x80)
			return -1;
#endif
		wc <<= 6;
		wc |= ps[-2] & 0x3f;
	need2:;
#ifdef UTFERR
		if ((ps[-1] & 0xc0) != 0x80)
			return -1;
#endif
		wc <<= 6;
		wc |= ps[-1] & 0x3f;
	need1:;
#ifdef UTFERR
		if ((ps[0] & 0xc0) != 0x80)
			return -1;
#endif
		wc <<= 6;
		wc |= ps[0] & 0x3f;
		*wp = wc;
		return ret;
	}
	else if ((wc = *wp) < 0xa0)
	{
		if (wc == SS2)
		{
			if ((len = eucw2) == 0)
				return 0;
			*wp = P01;
		}
		else if (wc == SS3)
		{
			if ((len = eucw3) == 0)
				return 0;
			*wp = P10;
		}
		else
			return 0;
	}
	else
	{
		if ((len = eucw1) == 0)
			return 0;
		*wp = P11;
		wc &= 0x7f;
		if (--len == 0)
		{
			*wp |= wc;
			return 1;
		}
	}
	ret = len;
	do
	{
#ifdef EUCERR
		if (ps[0] < 0x80)
			return -1;
#endif
		wc <<= 7;
		wc |= *ps++ & 0x7f;
	} while (--len != 0);
	*wp |= wc;
	return ret;
}

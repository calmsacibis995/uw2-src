/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/_wssize.c	1.4"
/*LINTLIBRARY*/

#include "synonyms.h"
#include "wcharm.h"

size_t
#ifdef __STDC__
_wssize(register const wchar_t *p, size_t nb, size_t nc)
#else
_wssize(p, nb, nc)register const wchar_t *p; size_t nb, nc;
#endif
{
	register wchar_t wc;
	register size_t size = 0;
	register int n;

	for (wc = *p; nc != 0; --nc, wc = *++p)	/* skip bad values */
	{
		switch ((wc >> EUCDOWN) & DOWNMSK)
		{
		default:
			continue;
		case DOWNP00:
			if ((wc & ~(wchar_t)0xff) != 0
				|| wc >= 0xa0 && multibyte && eucw1 != 0)
			{
				continue;
			}
			if (wc == 0)
				return size;
			if (++size > nb)
				return nb;
			continue;
		case DOWNP11:
			wc &= ~EUCMASK;
			switch (n = eucw1)
			{
			default:
#if MB_LEN_MAX > 5
				wc >>= 7 * (n - 1);
				break;
#endif
			case 0:
				continue;
			case 4:
				wc >>= 7 * 3;
				break;
			case 3:
				wc >>= 7 * 2;
				break;
			case 2:
				wc >>= 7 * 1;
				break;
			case 1:
				break;
			}
			if ((wc | 0x80) >= 0xa0)
				break;
			continue;
		case DOWNP01:
			if ((n = eucw2) != 0)
			{
				n++;
				break;
			}
			continue;
		case DOWNP10:
			if ((n = eucw3) != 0)
			{
				n++;
				break;
			}
			continue;
		}
		if ((size += n) > nb)
			return size - n;
	}
	return size;
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libw:port/wstring/wstostr.c	1.1.4.1"
#ident  "$Header: wstostr.c 1.2 91/06/26 $"

/*
 * Wstostr transforms process codes in wchar_t string "s2"
 * into EUC, and transfers those to character string "s1",
 * stopping after the wchar_t type NULL character has been
 * processed.
 */

#include <widec.h>
#include <errno.h>
#include <sys/euc.h>
#include "pcode.h"

#ifdef _WCHAR16
# define MY_EUCMASK	H_EUCMASK
# define MY_P11		H_P11
# define MY_P01		H_P01
# define MY_P10		H_P10
# define MY_SHIFT	8
#else
# define MY_EUCMASK	EUCMASK
# define MY_P11		P11
# define MY_P01		P01
# define MY_P10		P10
# define MY_SHIFT	7
#endif

char *
wstostr(s1, s2)
	register char *s1;
	const wchar_t *s2;
{
	register char *s;
	register wchar_t wchar;
	register int size, length;
	int cs1;
	char *os1 = s1;

	while ((wchar = *s2++) != 0)
	{
		switch (wchar & MY_EUCMASK)
		{
		default:
			if ((wchar & ~0377) || multibyte && wchar >= 0240)
			{
bad_seq:
				*s1 = '\0';
				errno = EILSEQ;
				return 0;
			}
			*s1++ = wchar;
			continue;
		case MY_P11:
			if ((size = eucw1) == 0)
				goto bad_seq;
			length = size;
			cs1 = 1;
			break;
		case MY_P01:
#ifdef _WCHAR16
			if (wchar < 0240 || !multibyte && wchar < 0400)
			{
				*s1++ = wchar;
				continue;
			}
#endif
			if ((size = eucw2) == 0)
				goto bad_seq;
			*s1 = SS2;
			length = size + 1;
			cs1 = 0;
			break;
		case MY_P10:
			if ((size = eucw3) == 0)
				goto bad_seq;
			*s1 = SS3;
			length = size + 1;
			cs1 = 0;
			break;
		}
		s = s1 + length;	/* fill in backwards */
		do
		{
			*--s = (wchar | 0200) & 0377;
			wchar >>= MY_SHIFT;
		} while (--size != 0);
		if (cs1 && (unsigned char)*s < 0240) /* C1 byte cannot be first */
			goto bad_seq;
		s1 += length;
	}
	*s1 = '\0';
	return os1;
}

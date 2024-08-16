/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/_outwc.c	1.4"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <errno.h>
#include "stdiom.h"

#define PUT(ch, fp) (--(fp)->_cnt < 0 ? (*fls)(ch, fp) : (*(fp)->_ptr++ = (ch)))

	/*
	* Returns:
	*	EOF if (*fls)() returns EOF.
	*	0   if wi has an invalid value.  The stream is untouched.
	*	n>0 otherwise.  n is the number of bytes output.
	*/
int
#ifdef __STDC__
_outwc(register wint_t wi, register FILE *fp, int (*fls)(int, FILE *))
#else
_outwc(wi, fp, fls)register wint_t wi; register FILE *fp; int (*fls)();
#endif
{
	Uchar mb[MB_LEN_MAX];
	register Uchar *p;
	register int n, len;

	switch ((wi >> EUCDOWN) & DOWNMSK)
	{
	default:
	bad:;
		errno = EILSEQ;
		return 0;
	case DOWNP00:
		if ((wi & ~(wchar_t)0xff) != 0
			|| wi >= 0xa0 && multibyte && eucw1 != 0)
		{
			goto bad;
		}
		if (PUT(wi, fp) == EOF)
			return EOF;
		len = 1;
		goto ret;
	case DOWNP11:
		len = eucw1;
		mb[0] = '\0';
		break;
	case DOWNP01:
		len = eucw2;
		mb[0] = SS2;
		break;
	case DOWNP10:
		len = eucw3;
		mb[0] = SS3;
		break;
	}
	switch (n = len)	/* fill in backwards */
	{
	case 0:
		goto bad;
#if MB_LEN_MAX > 5
	default:
		p = &mb[n + 1];
		do
		{
#if UCHAR_MAX == 0xff
			*--p = wi | 0x80;
#else
			*--p = (wi | 0x80) & 0xff;
#endif
			wi >>= 7;
		} while (--n != 0);
		n = len;
		break;
#endif /*MB_LEN_MAX > 5*/
	case 4:
#if UCHAR_MAX == 0xff
		mb[4] = wi | 0x80;
#else
		mb[4] = (wi | 0x80) & 0xff;
#endif
		wi >>= 7;
		/*FALLTHROUGH*/
	case 3:
#if UCHAR_MAX == 0xff
		mb[3] = wi | 0x80;
#else
		mb[3] = (wi | 0x80) & 0xff;
#endif
		wi >>= 7;
		/*FALLTHROUGH*/
	case 2:
#if UCHAR_MAX == 0xff
		mb[2] = wi | 0x80;
#else
		mb[2] = (wi | 0x80) & 0xff;
#endif
		wi >>= 7;
		/*FALLTHROUGH*/
	case 1:
#if UCHAR_MAX == 0xff
		mb[1] = wi | 0x80;
#else
		mb[1] = (wi | 0x80) & 0xff;
#endif
		break;
	}
	if (mb[0] == '\0')	/* code set 1 */
	{
		if (mb[1] < 0xa0)	/* meta-control cannot be first byte */
			goto bad;
	}
	else
	{
		if (PUT(mb[0], fp) == EOF)
			return EOF;
		len++;
	}
	p = &mb[0];
	do
	{
		if (PUT(*++p, fp) == EOF)
			return EOF;
	} while (--n != 0);
ret:;
	BUFSYNC(fp, (BFILE *)fp->_base);
	return len;
}

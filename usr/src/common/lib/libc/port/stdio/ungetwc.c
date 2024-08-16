/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/ungetwc.c	1.5"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak ungetwc = _ungetwc
#endif

wint_t
#ifdef __STDC__
ungetwc(wint_t wi, register FILE *fp)	/* pushback wide char on read stream */
#else
ungetwc(wi, fp)wint_t wi; register FILE *fp;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;
	Uchar mb[MB_LEN_MAX];
	register Uchar *p;
	register wint_t x;
	int len, n;

	if ((fp->_flag & (_IOREAD | _IORW)) == 0)
	{
		errno = EBADF;
		return EOF;
	}
	switch ((wi >> EUCDOWN) & DOWNMSK)
	{
	default:	/* includes EOF */
	bad:;
		errno = EILSEQ;
		return EOF;
	case DOWNP00:
		if ((wi & ~(wchar_t)0xff) != 0
			|| wi >= 0xa0 && multibyte && eucw1 != 0)
		{
			goto bad;
		}
		mb[0] = wi;
		n = 1;
		goto onebyte;
	case DOWNP11:
		if ((len = eucw1) == 0)
			goto bad;
		p = &mb[n = len];
		break;
	case DOWNP01:
		if ((len = eucw2) == 0)
			goto bad;
		mb[0] = SS2;
		p = &mb[n = len + 1];
		break;
	case DOWNP10:
		if ((len = eucw3) == 0)
			goto bad;
		mb[0] = SS3;
		p = &mb[n = len + 1];
		break;
	}
	/*
	* Fill in mb[] backwards.
	*/
	x = wi;
	do
	{
#if UCHAR_MAX == 0xff
		*--p = x | 0x80;
#else
		*--p = (x | 0x80) & 0xff;
#endif
		x >>= 7;
	} while (--len != 0);
	if (p == &mb[0])	/* code set 1 */
	{
		if (mb[0] < 0xa0)	/* meta-control cannot be first byte */
			goto bad;
	}
onebyte:;
	/*
	* Pushback bytes backwards, too.
	*/
	p = &mb[n];
	STDLOCK(&bp->lock);
	do 
	{
		if (fp->_ptr == bp->begptr)	/* at start of buffer */
		{
			if (bp->begptr == 0)
				bp = _findbuf(fp);
			p--;
		}
		else if (*--fp->_ptr == *--p)	/* unget same byte */
		{
			fp->_cnt++;
			fp->_flag &= ~_IOEOF;
			continue;
		}
		else if (bp->eflags & IO_PUSHED) /* pushback buffer with room */
		{
			*fp->_ptr = *p;
			fp->_cnt++;
			continue;
		}
		else
			fp->_ptr++;	/* restore value */
		if (_pushbuf(bp, *p) == EOF)
		{
			wi = EOF;
			break;
		}
		fp->_flag &= ~_IOEOF;
	} while (--n != 0);
	STDUNLOCK(&bp->lock);
	return wi;
}

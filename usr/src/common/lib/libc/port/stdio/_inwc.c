/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/_inwc.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <errno.h>
#include "stdiom.h"

#define GET(fp)	(--(fp)->_cnt < 0 ? (*fil)(fp) : *(fp)->_ptr++)

	/*
	* Returns:
	*	EOF if "initial byte" (already in *wp) is EOF.
	*		*wp = EOF.
	*	0   if a bad byte reached (only it is left unread).
	*		*wp = EOF; errno = EILSEQ.
	*	n>0 if n bytes form a valid multibyte character (including \0).
	*		*wp = "the wide character".
	*/
int
#ifdef __STDC__
_inwc(wint_t *wp, register FILE *fp, int (*fil)(FILE *))
#else
_inwc(wp, fp, fil)wint_t *wp; register FILE *fp; int (*fil)();
#endif
{
	register wint_t wi;
	register int len, ch, sz;

	if ((wi = *wp) == EOF)
		return EOF;
	if (wi < 0x80 || !multibyte)
	{
	onebyte:;
		sz = 1;
		goto ret;
	}
	if (wi < 0xa0)	/* C1 (i.e., metacontrol) byte */
	{
		if (wi == SS2)
		{
			if ((len = eucw2) == 0)
				goto onebyte;
			*wp = P01;
		}
		else if (wi == SS3)
		{
			if ((len = eucw3) == 0)
				goto onebyte;
			*wp = P10;
		}
		else
			goto onebyte;
		wi = 0;
		sz = len + 1;
	}
	else
	{
		if ((len = eucw1) == 0)
			goto onebyte;
		wi &= 0x7f;
		sz = len;
		if (--len == 0)
		{
			*wp = wi | P11;
			goto ret;
		}
		*wp = P11;
	}
	do /* fill in rest of the bytes */
	{
		if ((ch = GET(fp)) < 0x80)	/* includes EOF */
		{
			errno = EILSEQ;
			*wp = EOF;
			if (ch == EOF)	/* no need for BUFSYNC */
				return 0;
			fp->_ptr--;	/* always space to back up one byte */
			fp->_cnt++;
			sz = 0;
			goto ret;
		}
		wi <<= 7;
		wi |= ch & 0x7f;
	} while (--len != 0);
	*wp |= wi;
ret:;
	BUFSYNC(fp, (BFILE *)fp->_base);
	return sz;
}

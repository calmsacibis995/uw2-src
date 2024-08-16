/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/_unwc.c	1.1"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include "stdiom.h"

	/*
	* Pushback wide character onto stream.
	* Usually just a matter of an adjustment to _ptr and _cnt.
	* Only called from *wscanf's _iwdoscan.
	*/
void
#ifdef __STDC__
_unwc(register FILE *fp, wchar_t wc, int nbyte)
#else
_unwc(fp, wc, nbyte)register FILE *fp; wchar_t wc; int nbyte;
#endif
{
	register BFILE *bp = (BFILE *)fp->_base;
	int i;
	Uchar mb[MB_LEN_MAX];

	if ((i = fp->_ptr - bp->begptr) >= nbyte)
	{
		fp->_ptr -= nbyte;
		fp->_cnt += nbyte;
		return;
	}
	/*
	* The multibyte character crossed the buffer boundary:
	* Use the full pushback buffer scheme from unget[w]c().
	* Room for at least one multibyte character is guaranteed.
	*/
	fp->_ptr = bp->begptr;
	fp->_cnt += i;
	wctomb((char *)&mb[0], wc);
	nbyte -= i;
	if (_pushbuf(bp, mb[--nbyte]) == EOF)
		return;
	while (nbyte != 0)
	{
		*--fp->_ptr = mb[--nbyte];
		fp->_cnt++;
	}
}

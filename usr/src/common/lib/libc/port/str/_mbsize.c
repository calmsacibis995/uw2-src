/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/_mbsize.c	1.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <string.h>
#include <errno.h>
#include "wcharm.h"
#include "qstr.h"

size_t
#ifdef __STDC__
_mbsize(register const unsigned char *p0, size_t *nc)
#else
_mbsize(p0, nc)register const unsigned char *p0; size_t *nc;
#endif
{
	register const unsigned char *p, *q;
	register size_t cnt;
	register int ch;

	cnt = *nc;
	if (!multibyte) /* number of bytes is the same */
	{
#ifdef STREND
		cnt = STREND((char *)p0, cnt) - (char *)p0;
#else
		if ((p = (unsigned char *)memchr((char *)p0, '\0', cnt)) != 0)
			cnt = p - p0;
#endif
		*nc = cnt;
		return cnt;
	}
	for (p = p0; cnt != 0; ++p, --cnt)
	{
		if ((ch = *p) < 0x80) /* fits in 7 bits */
		{
			if (ch == '\0')
				break;
			continue;
		}
		else if (ch < 0xa0) /* metacontrol byte */
		{
			if (ch == SS2)
			{
				if ((ch = eucw2) == 0)
					continue;
			}
			else if (ch == SS3)
			{
				if ((ch = eucw3) == 0)
					continue;
			}
			else
				continue;
		}
		else if ((ch = eucw1) == 0 || --ch == 0)
			continue;
		q = p;
		do
		{
			if (*++p < 0x80)
			{
				p = q;
				errno = EILSEQ;
				goto err;
			}
		} while (--ch != 0);
	}
err:;
	*nc -= cnt;
	return p - p0;
}

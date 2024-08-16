/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/memshift.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"

char *
memshift(dst, src, len)
char	*dst;
char	*src;
int	len;
{
	register char	*d;
	register char	*s;
	register int	n;
	extern char	*memcpy();

	if (dst < src)
		if (dst + len <= src)
			memcpy(dst, src, len);
		else
			for (s = src, d = dst, n = len; n > 0; n--)
				*d++ = *s++;
	else
		if (src + len <= dst)
			return memcpy(dst, src, len);
		else
			for (n = len, s = src + n, d = dst + n; n > 0; n--)
				*--d = *--s;
	return dst;
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)zic:scheck.c	1.1.3.3"
#ident	"$Header: scheck.c 1.1 91/02/28 $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*LINTLIBRARY*/

#include <stdio.h>
#include <ctype.h>

extern char *	imalloc();

char *
scheck(string, format)
char *	string;
char *	format;
{
	register char *	fbuf;
	register char *	fp;
	register char *	tp;
	register int	c;
	register char *	result;
	char		dummy;

	result = "";
	if (string == NULL || format == NULL)
		return result;
	fbuf = imalloc(2 * strlen(format) + 4);
	if (fbuf == NULL)
		return result;
	fp = format;
	tp = fbuf;
	while ((*tp++ = c = *fp++) != '\0') {
		if (c != '%')
			continue;
		if (*fp == '%') {
			*tp++ = *fp++;
			continue;
		}
		*tp++ = '*';
		if (*fp == '*')
			++fp;
		while (isascii(*fp) && isdigit(*fp))
			*tp++ = *fp++;
		if (*fp == 'l' || *fp == 'h')
			*tp++ = *fp++;
		else if (*fp == '[')
			do *tp++ = *fp++;
				while (*fp != '\0' && *fp != ']');
		if ((*tp++ = *fp++) == '\0')
			break;
	}
	*(tp - 1) = '%';
	*tp++ = 'c';
	*tp = '\0';
	if (sscanf(string, fbuf, &dummy) != 1)
		result = format;
	free(fbuf);
	return result;
}

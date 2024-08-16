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
#ident	"@(#)fmli:sys/nstrcat.c	1.2.4.4"

#include	<varargs.h>
#include 	"sizes.h"
#include 	<string.h>

/*
 * useful for creating strings for error messages
 */
char *
nstrcat(va_alist)
va_dcl
{
	register char	*p, *q;
	static char	buf[MAX_WIDTH];
	va_list	ap;

	va_start(ap);
	for (q = buf; q < &buf[sizeof(buf) - 1] && (p = va_arg(ap, char *)); q += (int)strlen(q))
		strncpy(q, p, &buf[sizeof(buf) - 1] - q);
	va_end(ap);
	return buf;
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sccs:lib/mpwlib/cat.c	6.3"
#include	<varargs.h>

/*
	Concatenate strings.
 
	cat(destination,source1,source2,...,sourcen,0);
*/

/*VARARGS*/
char *
cat(va_alist)
va_dcl
{
	register char *d, *s, *dest;
	va_list ap;

	va_start(ap);
	dest = va_arg(ap, char *);
	d = dest;

	while (s = va_arg(ap, char *)) {
		while (*d++ = *s++) ;
		d--;
	}
	return (dest);
}

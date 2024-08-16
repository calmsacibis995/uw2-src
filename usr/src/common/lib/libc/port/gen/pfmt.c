/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/pfmt.c	1.7"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdarg.h>
#include <stdio.h>
#include "pfmtm.h"

#ifdef __STDC__
	#pragma weak pfmt = _pfmt
#endif

int
#ifdef __STDC__
pfmt(FILE *fp, long flags, const char *fmt, ...)
#else
/*VARARGS3*/
pfmt(fp, flags, fmt, va_alist)FILE *fp; long flags; const char *fmt; va_dcl
#endif
{
	va_list ap;
	int ret;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	ret = _ipfmt(fp, flags, fmt, ap);
	va_end(ap);
	return ret;
}

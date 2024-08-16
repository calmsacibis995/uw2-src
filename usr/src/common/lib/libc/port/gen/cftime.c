/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/cftime.c	1.17"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stddef.h>
#include <time.h>
#include <stdlib.h>

#ifdef __STDC__
	#pragma weak ascftime = _ascftime
	#pragma weak cftime = _cftime
#endif

int
#ifdef __STDC__
ascftime(char *buf, const char *fmt, const struct tm *ptm)
#else
ascftime(buf, fmt, ptm)char *buf; const char *fmt; const struct tm *ptm;
#endif
{
	if (fmt == 0 && (fmt = getenv("CFTIME")) == 0 || *fmt == '\0')
		fmt = "%N";
	return strftime(buf, ~(size_t)0, fmt, ptm);
}

int
#ifdef __STDC__
cftime(char *buf, const char *fmt, const time_t *tp)
#else
cftime(buf, fmt, tp)char *buf; const char *fmt; const time_t *tp;
#endif
{
	struct tm tmbuf;

	return ascftime(buf, fmt, localtime_r(tp, &tmbuf));
}

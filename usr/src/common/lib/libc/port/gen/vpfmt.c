/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/vpfmt.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdarg.h>
#include <stdio.h>
#include "pfmtm.h"

#ifdef __STDC__
	#pragma weak vpfmt = _vpfmt
#endif

int
#ifdef __STDC__
vpfmt(FILE *fp, long flags, const char *fmt, va_list ap)
#else
vpfmt(fp, flags, fmt, ap)FILE *fp; long flags; const char *fmt; va_list ap;
#endif
{
	return _ipfmt(fp, flags, fmt, ap);
}

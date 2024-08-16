/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/vlfmt.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdarg.h>
#include <stdio.h>
#include "pfmtm.h"

#ifdef __STDC__
	#pragma weak vlfmt = _vlfmt
#endif

int
#ifdef __STDC__
vlfmt(FILE *fp, long flags, const char *fmt, va_list ap)
#else
vlfmt(fp, flags, fmt, ap)FILE *fp; long flags; const char *fmt; va_list ap;
#endif
{
	struct s { va_list ap; } x; /* in case va_list is an array type */
	int ret;

	if (fp != 0)		/* prepare to reuse ap */
		x = *(struct s *)&ap;
	ret = _ipfmt((FILE *)0, flags, fmt, ap);
	if (fp != 0)
		ret = _ipfmt(fp, flags, fmt, x.ap);
	return ret;
}

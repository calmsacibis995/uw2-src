/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/putwchar.c	1.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak putwchar = _putwchar
#endif

wint_t
#ifdef __STDC__
putwchar(wchar_t wc)	/* add wide character to stdout */
#else
putwchar(wc)wchar_t wc;
#endif
{
	return fputwc(wc, stdout);
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/ctermid.c	1.17"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <string.h>
#include "stdiom.h"

#ifdef __STDC__
	#pragma weak ctermid = _ctermid
#endif

char *
#ifdef __STDC__
ctermid(char *s)
#else
ctermid(s)char *s;
#endif
{
	static char res[L_ctermid];

	return strcpy(s != 0 ? s : res, _str_devtty);
}

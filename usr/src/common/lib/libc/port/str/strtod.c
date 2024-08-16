/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strtod.c	1.4.2.1"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stddef.h>
#include "mkflt.h"

double
#ifdef __STDC__
strtod(const char *str, char **eptr)
#else
strtod(str, eptr)const char *str; char **eptr;
#endif
{
	MkFlt mf;

	mf.str = str;
	_mf_str(&mf);
	if (eptr != 0)
		*eptr = (char *)mf.str;
	_mf_tod(&mf);
	return mf.res.d;
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/atof.c	2.30"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stddef.h>
#include "mkflt.h"

double
#ifdef __STDC__
atof(const char *str)
#else
atof(str)const char *str;
#endif
{
	MkFlt mf;

	mf.str = str;
	_mf_str(&mf);
	_mf_tod(&mf);
	return mf.res.d;
}

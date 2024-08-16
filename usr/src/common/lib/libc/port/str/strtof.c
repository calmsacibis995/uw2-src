/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strtof.c	1.1"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stddef.h>
#include "mkflt.h"

#ifdef __STDC__
	#pragma weak strtof = _strtof
#endif

float
#ifdef __STDC__
strtof(const char *str, char **eptr)
#else
strtof(str, eptr)const char *str; char **eptr;
#endif
{
	MkFlt mf;

	mf.str = str;
	_mf_str(&mf);
	if (eptr != 0)
		*eptr = (char *)mf.str;
	_mf_tof(&mf);
	return mf.res.f;
}

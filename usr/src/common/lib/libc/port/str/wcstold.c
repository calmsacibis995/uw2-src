/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcstold.c	1.1"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stddef.h>
#include "mkflt.h"

#ifndef NO_LONG_DOUBLE

#ifdef __STDC__
	#pragma weak wcstold = _wcstold
#endif

long double
#ifdef __STDC__
wcstold(const wchar_t *wcs, wchar_t **eptr)
#else
wcstold(wcs, eptr)const wchar_t *wcs; wchar_t **eptr;
#endif
{
	MkFlt mf;

	mf.wcs = wcs;
	_mf_wcs(&mf);
	if (eptr != 0)
		*eptr = (wchar_t *)mf.wcs;
	_mf_told(&mf);
	return mf.res.ld;
}

#endif /*NO_LONG_DOUBLE*/

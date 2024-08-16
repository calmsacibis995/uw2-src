/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcstod.c	1.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stddef.h>
#include "mkflt.h"

#ifdef __STDC__
	#pragma weak wcstod = _wcstod
#endif

double
#ifdef __STDC__
wcstod(const wchar_t *wcs, wchar_t **eptr)
#else
wcstod(wcs, eptr)const wchar_t *wcs; wchar_t **eptr;
#endif
{
	MkFlt mf;

	mf.wcs = wcs;
	_mf_wcs(&mf);
	if (eptr != 0)
		*eptr = (wchar_t *)mf.wcs;
	_mf_tod(&mf);
	return mf.res.d;
}

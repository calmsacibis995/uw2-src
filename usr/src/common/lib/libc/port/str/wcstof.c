/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/wcstof.c	1.1"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stddef.h>
#include "mkflt.h"

#ifdef __STDC__
	#pragma weak wcstof = _wcstof
#endif

float
#ifdef __STDC__
wcstof(const wchar_t *wcs, wchar_t **eptr)
#else
wcstof(wcs, eptr)const wchar_t *wcs; wchar_t **eptr;
#endif
{
	MkFlt mf;

	mf.wcs = wcs;
	_mf_wcs(&mf);
	if (eptr != 0)
		*eptr = (wchar_t *)mf.wcs;
	_mf_tof(&mf);
	return mf.res.f;
}

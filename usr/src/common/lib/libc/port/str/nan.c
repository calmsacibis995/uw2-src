/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/nan.c	1.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stddef.h>
#include <limits.h>
#include <ctype.h>
#include "mkflt.h"

#ifdef __STDC__
	#pragma weak nan = _nan
	#pragma weak nanf = _nanf
#   ifndef NO_LONG_DOUBLE
	#pragma weak nanl = _nanl
#   endif
#endif

static void
#ifdef __STDC__
fill(MkFlt *mfp, const char *str)	/* fill in bits for specified NaN */
#else
fill(mfp, str)MkFlt *mfp; const char *str;
#endif
{
	register unsigned long val;
	register int ch, cnt;

	mfp->kind = MFK_VALNAN;
	mfp->ibi.next = 0;
	cnt = 0;
	val = 0;
	for (ch = *str;; ch = *++str)
	{
		if (!isxdigit(ch))
		{
			if (isalpha(ch) || ch == '_')
				continue;
			break;
		}
		if (++cnt > ULBITS / 4)
		{
			if (mfp->ibi.next >= NPKT)
				break;
			mfp->ibi.pkt[mfp->ibi.next++] = val;
			val = 0;
			cnt = 1;
		}
		val <<= 4;
		if (isdigit(ch))
			val |= ch - '0';
		else if (isupper(ch))
			val |= ch - 'A' + 10;
		else
			val |= ch - 'a' + 10;
	}
	if (mfp->ibi.next < NPKT)
	{
		if (cnt != 0 && (cnt = ULBITS / 4 - cnt) != 0)
			val <<= cnt * 4;
		mfp->ibi.pkt[mfp->ibi.next++] = val;
	}
}

double
#ifdef __STDC__
nan(const char *str)
#else
nan(str)const char *str;
#endif
{
	MkFlt mf;

	mf.bp = &mf.ibi;
	if (*str == '\0')
		mf.kind = MFK_DEFNAN;
	else
		fill(&mf, str);
	_mf_tod(&mf);
	return mf.res.d;
}

float
#ifdef __STDC__
nanf(const char *str)
#else
nanf(str)const char *str;
#endif
{
	MkFlt mf;

	mf.bp = &mf.ibi;
	if (*str == '\0')
		mf.kind = MFK_DEFNAN;
	else
		fill(&mf, str);
	_mf_tof(&mf);
	return mf.res.f;
}

#ifndef NO_LONG_DOUBLE

long double
#ifdef __STDC__
nanl(const char *str)
#else
nanl(str)const char *str;
#endif
{
	MkFlt mf;

	mf.bp = &mf.ibi;
	if (*str == '\0')
		mf.kind = MFK_DEFNAN;
	else
		fill(&mf, str);
	_mf_told(&mf);
	return mf.res.ld;
}

#endif /*NO_LONG_DOUBLE*/

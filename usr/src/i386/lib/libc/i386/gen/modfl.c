/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:gen/modfl.c	1.2"
/*LINTLIBRARY*/
/*
 * modfl(value, iptr) returns the signed fractional part of value
 * and stores the integer part indirectly through iptr.
 *
 */

#ifdef __STDC__
	#pragma weak modfl = _modfl
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <unistd.h>
#include <values.h>
#include <errno.h>
#include "fpparts.h"


long double
modfl(value, iptr)
long double value;
register long double *iptr;
{
	long double absvalue;

	if (NANorINFLD(value)) { /* check for NAN or INF (IEEE only) */
		long double	ret;
		if (!INFLD(value)) {	/* is a NaN */
			/* Raise an exception on non-quiet NaN. */
			SigNANLD(value);
			errno=EDOM;
			ret=value;
		} else
			ret = (value > 0) ? 0.0 : -0.0;
		*iptr = value;
		return ret;
	} else if (!value)
		return (*iptr = value);
	if ((absvalue = (value >= 0.0L) ? value : -value) >= LDMAXPOWTWO) {
		*iptr = value; /* it must be an integer */
	}
	else {
		*iptr = absvalue + LDMAXPOWTWO; /* shift fraction off right */
		*iptr -= LDMAXPOWTWO; /* shift back without fraction */
		while (*iptr > absvalue) /* above arithmetic might round */
			*iptr -= 1.0L; /* test again just to be sure */
		if (value < 0.0L)
			*iptr = -*iptr;
	}
	return (value - *iptr); /* signed fractional part */
}

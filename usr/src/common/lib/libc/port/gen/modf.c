/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/modf.c	1.10.3.1"
/*LINTLIBRARY*/
/*
 * modf(value, iptr) returns the signed fractional part of value
 * and stores the integer part indirectly through iptr.
 *
 * if value is a NaN returns error EDOM and value NaN and sets *iptr to NaN.
 *
 * if value is +-inf, returns +-0 and sets *iptr to +-inf.
 */

#ifdef __STDC__
	#pragma weak modf = _modf
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <values.h>
#include <errno.h>
#include "fpparts.h"

double
modf(value, iptr)
double value;
register double *iptr;
{
	register double absvalue;
	double ret;

	if (NANorINF(value)) { /* check for NAN or INF (IEEE only) */
		if (!INF(value)) {	/* is a NaN */
			/* Raise an exception on non-quiet NaN. */
			SigNAN(value);
			errno=EDOM;
			ret=value;
		} else
			ret = (value > 0) ? 0.0 : -0.0;
		*iptr = value;
		return ret;
	} else if (!value)
		return (*iptr = value);

	if ((absvalue = (value >= 0.0) ? value : -value) >= MAXPOWTWO)
		*iptr = value; /* it must be an integer */
	else {
		*iptr = absvalue + MAXPOWTWO; /* shift fraction off right */
		*iptr -= MAXPOWTWO; /* shift back without fraction */
		while (*iptr > absvalue) /* above arithmetic might round */
			*iptr -= 1.0; /* test again just to be sure */
		if (value < 0.0)
			*iptr = -*iptr;
	}
	return (value - *iptr); /* signed fractional part */
}

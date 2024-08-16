/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/modff.c	1.8"
/*LINTLIBRARY*/
/*
 * modff(value, iptr) returns the signed fractional part of value
 * and stores the integer part indirectly through iptr.
 *
 */

#ifdef __STDC__
	#pragma weak modff = _modff
#endif
#include <sys/types.h>
#include "synonyms.h"
#include <values.h>
#include "fpparts.h"
#include <errno.h>

float
#ifdef __STDC__
modff(float value, register float *iptr)
#else
modff(value, iptr)
float value;
register float *iptr;
#endif
{
	register float absvalue;

	if (FNANorINF(value)) { /* check for NAN or INF (IEEE only) */
		float	ret;
		if (!FINF(value)) {	/* is a NaN */
			/* Raise an exception on non-quiet NaN. */
			FSigNAN(value);
			errno=EDOM;
			ret=value;
		} else
			ret = (value > (float)0.0) ? (float)0.0 : 
				(float)-0.0;
		*iptr = value;
		return ret;
	} else if (!value)
		return (*iptr = value);
	if ((absvalue = (value >= (float)0.0) ? value : -value) >= FMAXPOWTWO)
		*iptr = value; /* it must be an integer */
	else {
		*iptr = absvalue + FMAXPOWTWO; /* shift fraction off right */
		*iptr -= FMAXPOWTWO; /* shift back without fraction */
		while (*iptr > absvalue) /* above arithmetic might round */
			*iptr -= (float)1.0; /* test again just to be sure */
		if (value < (float)0.0)
			*iptr = -*iptr;
	}
	return (value - *iptr); /* signed fractional part */
}

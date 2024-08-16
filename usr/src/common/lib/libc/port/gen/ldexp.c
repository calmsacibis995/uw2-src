/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/ldexp.c	2.12.2.3"
/*LINTLIBRARY*/
/*
 *	double ldexp (value, exp)
 *		double value;
 *		int exp;
 *
 *	Ldexp returns value * 2**exp, if that result is in range.
 *	
 *	If value is a NaN, returns error EDOM and value NaN.  If value is
 *	also a signalling NaN, raises an invalid op exception.
 *
 *	On underflow, return error ERANGE and value 0.
 *
 *	In -Xt mode,
 *	On overflow, return error ERANGE and value +-HUGE.
 *
 *	In -Xa and -Xc modes,
 *	On overflow, return error ERANGE and value +-HUGE_VAL.
 */

#include "synonyms.h"
#include <values.h>
#include <math.h>
#include <errno.h>
#include "fpparts.h"
/* Largest signed long int power of 2 */
#define MAXSHIFT	(BITSPERBYTE * sizeof(long) - 2)

extern double frexp();

double
ldexp(arg, exp)
double arg;
register int exp;
{
	int old_exp;
	register double value;

	if (NANorINF(arg) && !INF(arg)){	/* ie x is NaN */

		/* raise an exception if not a quiet NaN */
		SigNAN(arg);

		errno=EDOM;
		return (arg);
	}

	value = arg;
	if (exp == 0 || value == 0.0) /* nothing to do for zero */
		return (value);

#if	!(M32)	/* "cc" can't handle cast of double to void on M32 */
	(void)
#endif
	frexp(value, &old_exp);
	if (exp > 0) {
		if (exp + old_exp > MAXBEXP) { /* overflow */
			errno = ERANGE;
			if (_lib_version == c_issue_4)
				return (value < 0 ? -HUGE : HUGE);
			else
				return (value < 0 ? -HUGE_VAL : HUGE_VAL);
		}
		for ( ; exp > MAXSHIFT; exp -= MAXSHIFT)
			value *= (unsigned)(1L << MAXSHIFT);
		return (value * (1L << exp));
	}
	if (exp + old_exp < MINBEXP) { /* underflow */
		errno = ERANGE;
		return (0.0);
	}
	for ( ; exp < -MAXSHIFT; exp += MAXSHIFT)
		value *= 1.0/(unsigned)(1L << MAXSHIFT); /* mult faster than div */
	return (value / (1L << -exp));
}

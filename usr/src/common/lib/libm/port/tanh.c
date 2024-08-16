/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/tanh.c	1.2"
/*LINTLIBRARY*/
/*
 *	tanh returns the hyperbolic tangent of its double-precision argument.
 *	It calls exp for absolute values of the argument > ~0.55.
 *	Algorithm and coefficients from Cody and Waite (1980).
 *
 *	If x is a NaN, return error EDOM and value NaN.  If x is also a 
 *	signalling NaN, raise an invalid op exception.
 */

#include <math.h>
#include <values.h>
#include <errno.h>
#include "fpparts.h"

#ifndef __STDC__
#define const
#endif

#define X_BIG	(0.5 * (LN_MAXDOUBLE + M_LN2))
#define LN_3_2	0.54930614433405484570

double
tanh(arg)
double arg;
{
	register int neg = 0;
	register double x;

	if (NANorINF(arg)) {
		if (!INF(arg))	 	/* arg is a NaN */
			return _domain_err(arg,0.0,arg,"tanh",4);
		else			/* arg is inf */
			return (arg < 0) ? -1.0 : 1.0;
	}

	if (arg < 0) {
		x = -arg;
		neg++;
	} else
		x = arg;
	if (x > X_BIG)
		x = 1.0;
	else if (x > LN_3_2) {
		x = 0.5 - 1.0/(exp(x + x) + 1.0); /* two steps recommended */
		x += x;
	} else if (x > X_EPS) { /* skip for efficiency and to prevent underflow */
		static const double p[] = {
			-0.96437492777225469787e0,
			-0.99225929672236083313e2,
			-0.16134119023996228053e4,
		}, q[] = {
			 1.0,
			 0.11274474380534949335e3,
			 0.22337720718962312926e4,
			 0.48402357071988688686e4,
		};
		register double y = x * x;

		x += x * y * _POLY2(y, p)/_POLY3(y, q);
	}
	return (neg ? -x : x);
}

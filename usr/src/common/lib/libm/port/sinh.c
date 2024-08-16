/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/sinh.c	1.6"
/*LINTLIBRARY*/
/*
 *	sinh returns the hyperbolic sine of its double-precision argument.
 *	A series is used for arguments smaller in magnitude than 1.
 *	The exponential function is used for arguments
 *	greater in magnitude than 1.
 *
 *	cosh returns the hyperbolic cosine of its double-precision argument.
 *	cosh is computed from the exponential function for
 *	all arguments.
 *
 *	Returns ERANGE error and value HUGE_VAL (or -HUGE_VALfor sinh of
 *	negative argument) if the correct result would overflow.
 *	(-Xt returns HUGE, not HUGE_VAL)
 *
 *	Returns EDOM error and
 *	value NaN if argument is a NaN
 *
 *	An invalid op exception is raised if argument
 *	is a signalling NaN.
 *
 *	Algorithm and coefficients from Cody and Waite (1980).
 */

#include <math.h>
#include <values.h>
#include <errno.h>
#include "fpparts.h"
#define X_MAX	(LN_MAXDOUBLE + M_LN2)
#define LNV	0.6931610107421875
#define V2M1	0.13830277879601902638e-4

static double sinh_exc();

double
sinh(x)
double x;
{
	register double y;
	register int sign = 0;

	if (NANorINF(x))
		return (sinh_exc(x, 1));

	if (x < 0.0) {
		y = -x;
		sign++;
	}
	else y = x;
	if (y <= 1) {
		static double p[] = {
			-0.78966127417357099479e0,
			-0.16375798202630751372e3,
			-0.11563521196851768270e5,
			-0.35181283430177117881e6,
		}, q[] = {
			 1.0,
			-0.27773523119650701667e3,
			 0.36162723109421836460e5,
			-0.21108770058106271242e7,
		};

		if (y < X_EPS) /* for efficiency and to prevent underflow */
			return (x);
		y = x * x;
		return (x + x * y * _POLY3(y, p)/_POLY3(y, q));
	}
	if (y > LN_MAXDOUBLE) /* exp(x) would overflow */
		return (sinh_exc(x, 1));
	x = exp(y);
	y = (0.5 * (x - 1.0/x));
	return(sign ? -y : y);
}

double
cosh(x)
double x;
{
	register double	y;

	if (NANorINF(x))
		return (sinh_exc(x, 0));

	if ((y=_ABS(x)) > LN_MAXDOUBLE) /* exp(x) would overflow */
		return (sinh_exc(x, 0));
	x = exp(y);
	return (0.5 * (x + 1.0/x));
}

/*
 * Either x is a NaN or overflow will occur.
 */

static double
sinh_exc(x, sinhflag)
double x;
register int sinhflag;
{
	double q1 = 0.0;
	double q2 = 0.0;
	double y;
	int neg = 0; /* sinh of negative argument */
	struct exception exc;

	exc.name = sinhflag ? "sinh" : "cosh";
	exc.arg1 = x;

	if (NANorINF(x) && !INF(x)) {
		return _domain_err(x,y,x,exc.name,4);
	}
	
	if (x < 0.0)
	{
		y = -x;
		if (sinhflag)
			neg = 1;
	}
	else 
		y = x;

	if (y < X_MAX) { /* result is still representable */
		x = exp(y - LNV);
		x += V2M1 * x;
		return (neg ? -x : x);
	}

	exc.type = OVERFLOW;
	if (_lib_version == c_issue_4)
		exc.retval = neg ? -HUGE : HUGE;
	else
		exc.retval = neg ? -HUGE_VAL : HUGE_VAL;

	if (_lib_version != c_issue_4 || !matherr(&exc))
		errno = ERANGE;
	return (exc.retval);
}

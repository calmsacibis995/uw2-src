/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/sinhf.c	1.5"
/*LINTLIBRARY*/
/*
 *	Single Precision Routine - float args and float return.
 *	sinh returns the hyperbolic sine of its single-precision argument.
 *	A series is used for arguments smaller in magnitude than 1.
 *	The exponential function is used for arguments
 *	greater in magnitude than 1.
 *
 *	cosh returns the hyperbolic cosine of its single-precision argument.
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

#define X_MAX	(float)(LN_MAXFLOAT + M_LN2)
#define LNV	(float)0.6931610107421875
#define V2M1	(float)0.13830277879601902638e-4

static float sinh_exc(float, register int);

static const float one = 1.0;
static const float half = 0.5;
static const float zero = 0.0;

float
sinhf(float x)
{
	register float y;
	register int sign = 0;

	if (FNANorINF(x))
		return (sinh_exc(x, 1));
	if (x < zero) {
		y = -x;
		sign++;
	}
	else y = x;

	if (y <= one) {
		static const double p[] = {
			-0.190333399e0,
			-0.713793159e1,
		}, q[] = {
			 1.0,
			-0.428277109e2,
		};

		if (y < FX_EPS) /* for efficiency and to prevent underflow */
			return (x);
		y = x * x;
		return (x + x * y * _POLY1(y, p)/_POLY1(y, q));
	}
	if (y > LN_MAXFLOAT) /* exp(x) would overflow */
		return (sinh_exc(x, 1));
	x = expf(y);
	y = (half * (x - one/x));
	return(sign ? -y : y);
}

float
coshf(float x)
{

	float	y;

	if (FNANorINF(x))
		return (sinh_exc(x, 0));
	y = _ABS(x);

	if (y > LN_MAXFLOAT) /* expf(x) would overflow */
		return (sinh_exc(x, 0));
	x = expf(y);
	return (half * (x + one/x));
}

/* x is a NaN or inifinity overflow will occcur */

static float
sinh_exc(float x, register int sinhflag)
{
	struct exception exc;
	int	neg = 0;
	float	y;

	exc.name = sinhflag ? "sinhf" : "coshf";

	if (FNANorINF(x) && !FINF(x)) {
		return _float_domain(x,y,x,exc.name,5);
	}
	if (x < 0.0F)
	{
		y = -x;
		if (sinhflag)
			neg = 1;
	}
	else
		y = x;
		
	if (y < X_MAX) { /* result is still representable */
		x = expf(y - LNV);
		x += V2M1 * x;
		return (neg ? -x : x);
	}
	if (_lib_version == c_issue_4) {
		exc.type = OVERFLOW;
		exc.arg1 = (double)x;
		exc.retval = neg ? -HUGE : HUGE;
		if (!matherr(&exc))
			errno = ERANGE;
		return (float)(exc.retval);
	}

	errno = ERANGE;
	return(neg ? (float)-HUGE_VAL : (float)HUGE_VAL);
}

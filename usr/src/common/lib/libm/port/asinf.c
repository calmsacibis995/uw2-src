/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/asinf.c	1.6"
/*LINTLIBRARY*/
/*
 *	Single Precision Routine - float args and float return.
 *	C program for single-precision asin/acos.
 *	Returns EDOM error and value 0 if |argument| > 1.
 *	Algorithm and coefficients from Cody and Waite (1980).
 *	Calls sqrt if |argument| > 0.5.
 *
 *	Returns EDOM error and
 *	value NaN if argument is a NaN or if |argument| > 1.
 * 	(-Xt returns 0 for |x| > 1)
 *
 *	An invalid op exception is raised if |argument| > 1 or argument
 *	is a signalling NaN.
 */

#include "synonyms.h"
#include <math.h>
#include <values.h>
#include <errno.h>
#include "fpparts.h"

#define FM_PI	(float)M_PI
#define FM_PI_2	(float)M_PI_2

static float asc_err(float, int);

static float
asin_acos(float x, int acosflag)
{
	register float y;
	register int neg = 0, large = 0;
	struct exception exc;
	
	static const float zero = 0.0;
	static const float one = 1.0;
	static const float half = 0.5;
	
	if (FNANorINF(x))
		return (asc_err(x, acosflag));

	exc.arg1 = (double)x;
	if (x < zero) {
		x = -x;
		neg++;
	}
	if (x > one) {
		return (asc_err(x, acosflag));
	}
	if (x > FX_EPS) { /* skip for efficiency and to prevent underflow */
		static const float p[] = {
			-0.504400557e0,
			 0.933935835e0,
		}, q[] = {
			 1.0,
			-0.554846723e1,
			 0.560363004e1,
		};

		if (x <= half)
			y = x * x;
		else {
			large++;
			y = half - half * x;
			x = -sqrtf(y);
			x += x;
		}
		x += x * y * _POLY1(y, p)/_POLY2(y, q);
	}
	if (acosflag) {
		if (!neg)
			x = -x;
		return (!large ? FM_PI_2 + x : neg ? FM_PI + x : x);
	}
	if (large)
		x += FM_PI_2;
	return (neg ? -x : x);
}

float
asinf(float x)
{
	return (asin_acos(x, 0));
}

float
acosf(float x)
{
	return (asin_acos(x, 1));
}

static float
asc_err(float x, int acosflag)
{
	float q1 = 0.0F;
	float q2 = 0.0F;
	float ret;

	/* 
	 * if x wasn't a NaN, make one for the return value,
	 * and raise an invalid op exception.
	 */

#if _IEEE
	if (!FNANorINF(x) || FINF(x)) {
		if (_lib_version == c_issue_4) 	
			ret = 0.0F;
		else
			FMKNAN(ret);

		q1 /= q2;
	} 
	else
		ret = x;
#else
	ret = 0.0F;
#endif

	return(_float_domain(x, 0.0F, ret,
		acosflag ? "acosf" : "asinf", 5));
}

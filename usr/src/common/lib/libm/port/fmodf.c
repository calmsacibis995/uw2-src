/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/fmodf.c	1.10"
/*LINTLIBRARY*/

/* fmodf(x,y)
 * single precision fmod
 * Return f =x-n*y, for some integer n, where f has same sign
 * as x and |f| < |y|
 *
 *  for y == 0, f = 0 
 * This implementation calculates IEEE rem and uses that to calculate
 * fmod
 * If x or y are NaN, return error EDOM and value NaN.  If x or y are also
 * signalling NaNs, raise an invalid op exception.
 *
 * If x is +-inf raise an invalid op exception and return error EDOM 
 * and value NaN.
 *
 * If y is +-inf or x is 0, return x.
 *
 * In -Xt mode,
 * If y is 0, raise the invalid op exception and return error EDOM and value
 * x.
 *
 * In -Xa and -Xc modes,
 * if y is 0, raise the invalid op exception and return error EDOM and value
 * NaN.
 */

#include "synonyms.h"
#include <math.h>
#include <values.h>
#include <errno.h>
#include "fpparts.h"

float	fmodf(float x, float y)
{
	float	r;

#if _IEEE
	if (FNANorINF(x)|| (FNANorINF(y) && !FINF(y)) || !y ) {
		float q1 = 0.0F;
		float q2 = 0.0F;
		float ret;

		if (FNANorINF(x) && !FINF(x)) 		/* x is a NaN */
			ret = x;
		else if (FNANorINF(y) && !FINF(y))	/* y is a NaN */
			ret = y;
		else {
			/* raise exception */
			q1 /= q2;
			ret = x;
			if (_lib_version != c_issue_4 || y)
				FMKNAN(ret);
		}

		return _float_domain(x,y,ret,"fmodf",5);

	} else if (!x || NANorINF(y))
		return x;
#else
	if (!y)
		return(_float_domain(x, y, 0.0, "fmodf, 5);
	if (!x)
		return x;
#endif
	if (y < (float)0.0)
		y = -y;
	r = (float)remainder(x, y);
	/*
	 * At this point we have rem(x,y)
	 * to get fmodf(x,y), we test the signs of x and of the
	 * remainder - if they're the same, we are done,
	 * else if x is negative, replace remainder with remainder -|y|,
	 * if x positive with remainder + |y| 
	 */
	if (x < (float)0.0) {
		if (r > (float)0.0)
			r -= y;
	}
	else { 
		if (r < (float)0.0)
			r += y;
	
	}
	return r;
}

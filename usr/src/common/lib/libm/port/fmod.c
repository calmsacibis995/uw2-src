/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/fmod.c	1.12"
/*LINTLIBRARY*/

/* fmod(x,y)
 * Return f =x-n*y, for some integer n, where f has same sign
 * as x and |f| < |y|
 *
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
 *
 * This implementation calculates IEEE rem and uses that to calculate
 * fmod
 */

#include "synonyms.h"
#include <math.h>
#include <errno.h>
#include <values.h>
#include "fpparts.h"

double	fmod(x, y)
double	x, y;
{
	register double	r;
	double ret;

#ifdef _IEEE
	if (NANorINF(x)|| (NANorINF(y) && !INF(y)) || y == 0.0 ) {
		double q1 = 0.0;
		double q2 = 0.0;

		if (NANorINF(x) && !INF(x)) 		/* x is a NaN */
			ret = x;
		else if (NANorINF(y) && !INF(y))	/* y is a NaN */
			ret = y;
		else {
			/* raise exception */
			q1 /= q2;
			ret = x;
			if (_lib_version != c_issue_4 || y != 0.0)
				MKNAN(ret);
		}

		return _domain_err(x,y,ret,"fmod",4);

	} else if (x == 0.0 || NANorINF(y))
		return x;
#else
	if (y == 0.0)
		return _domain_err(x,y,0.0,"fmod",4);
	else if (x == 0.0)
		return x;
#endif

	if (y < 0.0)
		y = -y;
	r = remainder(x, y);
	/*
	 * At this point we have remainder(x,y)
	 * to get fmod(x,y), we test the signs of x and of the
	 * remainder - if they're the same, we are done,
	 * else if x is negative, replace remainder with remainder -|y|,
	 * if x positive with remainder + |y| 
	 */
	if (x < 0.0) {
		if (r > 0.0)
			r -= y;
	}
	else { 
		if (r < 0.0)
			r += y;
	
	}
	return r;
}

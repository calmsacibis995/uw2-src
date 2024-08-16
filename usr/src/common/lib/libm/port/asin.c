/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/asin.c	1.7"
/*LINTLIBRARY*/
/*
 *	C program for double-precision asin/acos.
 *	Returns EDOM error and value 0 if |argument| > 1 in -Xt mode
 *	Algorithm and coefficients from Cody and Waite (1980).
 *	Calls sqrt if |argument| > 0.5.
 *
 *	Returns EDOM error and
 *	value NaN if argument is a NaN or if |argument| > 1.
 *	(-Xt returns 0 if |arg| >1)
 *
 *	An invalid op exception is raised if |argument| > 1 or argument
 *	is a signalling NaN.
 */

#include "synonyms.h"
#include <math.h>
#include <values.h>
#include <errno.h>
#include "fpparts.h"

#ifndef __STDC__
#define const
#endif

static double
asc_err(x,acosflag)
double x;
int acosflag;
{
	double q1 = 0.0;
	double q2 = 0.0;
	double ret = x;

#ifdef _IEEE
	/* 
	 * if x wasn't a NaN, make one for the return value,
	 * and raise an invalid op exception.
	 */
	if (!NANorINF(x) || INF(x)) {
		if (_lib_version == c_issue_4) 	
			ret = 0.0;
		else
			MKNAN(ret);

		q1 /= q2;
#else
		ret = 0.0;
#endif
	} 

	return _domain_err(x,0.0,ret,acosflag ? "acos" : "asin", 4);

}

static double
asin_acos(arg, acosflag)
double arg;	
int acosflag;
{
	register double x;
	register double y;
	register int neg = 0, large = 0;
	
	if (NANorINF(arg))
		return (asc_err(arg,acosflag));

	if (arg < 0.0) {  
		x = -arg;
		neg++;
	} else
		x = arg;

	if (x>1.0)
		return (asc_err(arg,acosflag));

	if (x > X_EPS) { /* skip for efficiency and to prevent underflow */
		static const double p[] = {
			-0.69674573447350646411e0,
			 0.10152522233806463645e2,
			-0.39688862997504877339e2,
			 0.57208227877891731407e2,
			-0.27368494524164255994e2,
		}, q[] = {
			 1.0,
			-0.23823859153670238830e2,
			 0.15095270841030604719e3,
			-0.38186303361750149284e3,
			 0.41714430248260412556e3,
			-0.16421096714498560795e3,
		};

		if (x <= 0.5)
			y = x * x;
		else {
			large++;
			y = 0.5 - 0.5 * x;
			x = -sqrt(y);
			x += x;
		}
		x += x * y * _POLY4(y, p)/_POLY5(y, q);
	}
	if (acosflag) {
		if (!neg)
			x = -x;
		return (!large ? M_PI_2 + x : neg ? M_PI + x : x);
	}
	if (large)
		x += M_PI_2;
	return (neg ? -x : x);
}


double
asin(x)
double x;
{
	return (asin_acos(x, 0));
}

double
acos(x)
double x;
{
	return (asin_acos(x, 1));
}

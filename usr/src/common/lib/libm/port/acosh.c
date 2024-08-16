/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/acosh.c	1.10"
/*LINTLIBRARY*/
/* 
 */

/* ACOSH(X)
 * acosh(x) returns the inverse hyperbolic cosine of x
 *
 * Method :
 *	Based on 
 *		acosh(x) = log [ x + sqrt(x*x-1) ]
 *	we have
 *		acosh(x) := log1p(x/2)+log4,if (x > MAXDOUBLE/2); 
 *		acosh(x) := log1p(2*x)		if (x > 1.0E20)
 *		acosh(x) := log1p( sqrt(x-1) * (sqrt(x-1) + sqrt(x+1)) ) .
 *	These formulae avoid the over/underflow complication.
 *
 *	In -Xa and -Xc modes, returns EDOM error and
 *	value NaN if argument is a NaN or less than 1.
 *
 *	An invalid op exception is raised if argument
 *	is a signalling NaN or less than 1.
 *
 * Note log(2*x)=log(x/n)+log(2*n) for large x.
 *  	  n=2 for IEEE double precision gives small addition roundoff
 *	  (n=1048576 for VAX double precision)
 */

#include "synonyms.h"
#include <math.h>
#include <errno.h>
#include <values.h>
#include "fpparts.h"

#define	BIG	1.0E20		/* 1 + BIG == BIG */


static const double ln4=1.386294361119890618834464242916;
double acosh(x)
double x;
{	
	double _log1p();
	register double t;

	if (NANorINF(x)) {
		if (INF(x)) {
			/* +inf returns +inf ; -inf falls through
			 * to <1.0 case 
			 */
			if (x > 0.0)
				return x;
		}
		else return (_domain_err(x, 0.0, x, "acosh", 5));
	}
	if (x < 1.0) {
		double	p;
		double q1 = 0.0, q2 = 0.0;
		MKNAN(p);
#if _IEEE
		/* raise invalid op exception */
		q1 /= q2;
#endif
		return (_domain_err(x, 0.0, p, "acosh", 5));
	}

	if (x >= MAXDOUBLE/2) {
		return(_log1p(x/2.0)+ln4);
	} else if(x > BIG ){
		/* x*2 will not overflow */
		return _log1p(x*2);
	}
	t = sqrt(x - 1.0);
	return(_log1p(t * (t + sqrt(x + 1.0))));
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/exp.c	1.5"
/*LINTLIBRARY*/
/* 
 */

/* exp(x)
 * return the exponential of x
 * Method:
 *	1. Argument Reduction: given the input x, find r and integer k such 
 *	   that
 *	                   x = k*ln2 + r,  |r| <= 0.5*ln2 .  
 *	   r will be represented as r := z+c for better accuracy.
 *
 *	2. Compute expm1(r)=exp(r)-1 by 
 *
 *			expm1(r=z+c) := z + exp__E(z,r)
 *
 *	3. exp(x) = 2^k * ( expm1(r) + 1 ).
 *
 *	If x is a NaN, return error EDOM and value NaN.  If x is also a
 *	signalling NaN, raise an invalid op exception.
 *
 *	On underflow, return error ERANGE and value 0.
 *
 *	In -Xt mode,
 *	On overflow, return error ERANGE and value +HUGE.
 *
 *	In -Xa and -Xc modes,
 *	On overflow, return error ERANGE and value +HUGE_VAL.
 */

#include <math.h>
#include <errno.h>
#include <values.h>
#include "fpparts.h"

#ifndef __STDC__
#define const
#endif

#if _IEEE
static const double
ln2hi  =  6.9314718036912381649E-1    , 
ln2lo  =  1.9082149292705877000E-10   ,
invln2 =  1.4426950408889633870E0     ;
#endif

#define SMALL	1.0E-19

/* coefficients for exp__E() */
#if _IEEE
static const double 
p1 = 1.3887401997267371720E-2,
p2 = 3.3044019718331897649E-5, 
q1 = 1.1110813732786649355E-1,
q2 = 9.9176615021572857300E-4;
#endif

static double
exp_err(x)
double x;
{
struct exception exc;

  exc.arg1 = x;
  exc.name = "exp";

  if (NANorINF(x) && !INF(x))		/* NaN */
	return _domain_err(x,0.0,x,"exp",3);
  else if ( x < 0 ){
	exc.type = UNDERFLOW;
	exc.retval = 0.0;
  } else {
	exc.type = OVERFLOW;
	exc.retval = _lib_version == c_issue_4 ? HUGE : HUGE_VAL;
  }

  if (_lib_version != c_issue_4 || !matherr(&exc))
	errno = ERANGE;

  return (exc.retval);
}

double exp(x)
double x;
{
	register double z, z1, c ;
	double hi,lo, p3, q;
	double xp, xh, w;
	int n, k;

	if (NANorINF(x))
		return(exp_err(x));

	if (x <= LN_MINDOUBLE) {
		if (x == LN_MINDOUBLE) /* protect against roundoff */
			return (MINDOUBLE); /* causing ldexp to underflow */
		else
			return(exp_err(x));
	}
	if (x >= LN_MAXDOUBLE) {
		if (x == LN_MAXDOUBLE) /* protect against roundoff */
			return (MAXDOUBLE); /* causing ldexp to overflow */
		else
			return(exp_err(x));
	}
	/* argument reduction : x --> x - k*ln2 */
	k = invln2 * x + (x < 0.0 ? -0.5 : 0.5);	/* k=NINT(x/ln2) */
	/* express x-k*ln2 as z+c */
	hi = x - k * ln2hi;
	z = hi - (lo = k * ln2lo);
	c = (hi - z) - lo;

	/* return 2^k*[expm1(x) + 1]  */
/* calculate exp__E(z,c) 
 * where 
 *			 /  exp(x+c) - 1 - x ,  1E-19 < |x| < .3465736
 *       exp__E(x,c) = 	| 		     
 *			 \  0 ,  |x| < 1E-19.
 *
 * Method:
 *	1. Rational approximation. Let r=x+c.
 *	   Based on
 *                                   2 * sinh(r/2)     
 *                exp(r) - 1 =   ----------------------   ,
 *                               cosh(r/2) - sinh(r/2)
 *	   exp__E(r) is computed using
 *                   x*x            (x/2)*W - ( Q - ( 2*P  + x*P ) )
 *                   --- + (c + x*[---------------------------------- + c ])
 *                    2                          1 - W
 * 	   where  P := _POLY2(x^2, p1)
 *	          Q := _POLY2(x^2, q1)
 *	          W := x/2-(Q-x*P),
 *
 *	    and cosh :
 *		sinh(r/2) =  r/2 + r * P  ,  cosh(r/2) =  1 + Q . )
 */
 
	if (_ABS(z) > SMALL) { 
           z1 = z * z;
	   p3 = z1 * (p1 + z1 * p2);
#if _IEEE
           q = z1 * (q1 + z1 * q2);
#endif
           xp = z * p3; 
	   xh = z * 0.5;
           w = xh - (q - xp);
	   p3 += p3;
	   c += z * ((xh * w - (q - (p3 + xp)))/(1.0 - w) + c);
	   z += (z1 * 0.5 + c);
	}
#if _IEEE /*in-line expansion of ldexp, but call function if argument
	  * de-normal 
	  */
	w = z + 1.0;
	if (((n = EXPONENT(w)) == 0) || (n + k <= 0))
		return(ldexp(w, k));
	EXPONENT(w) = n + k;
	return(w);
#else
	return(ldexp(z + 1.0, k));
#endif
}

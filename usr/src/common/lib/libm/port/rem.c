/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/rem.c	1.13"
/*LINTLIBRARY*/
/* 
 */

/* remainder is an IEEE required function - two implementations 
 * are provided:
 * one which works only on IEEE architectures, and one which works
 * on both IEEE and non-IEEE machines
 */

/* remainder(x,y)
 * Return x remainder y =x-n*y, n=[x/y] rounded (rounded to even 
 * in the half way case)
 *
 * If x or y are NaN, return error EDOM and value NaN.  If x or y are also
 * signalling NaNs, raise an invalid op exception.
 *
 * If x is +-inf raise an invalid op exception and return error EDOM 
 * and value NaN.
 *
 * If y is +-inf or x is 0, return x.
 *
 * If y is 0, raise the invalid op exception and return error EDOM and value
 * NaN.
 */

#ifdef __STDC__
	#pragma weak remainder = _remainder
#endif

#include "synonyms.h"
#include <math.h>
#include <errno.h>
#include <values.h>

#if _IEEE
#include "fpparts.h"
#define P754_NOFAULT 1
#include <ieeefp.h>


double	remainder(x, y)
double	x, y;
{

	double	hy, y2, t, t1;
	short	k;
	long	n;
	fp_except	mask;
	unsigned short	xexp, yexp, nx, nf, sign;

	if (NANorINF(x)|| (NANorINF(y) && !INF(y)) || !y ) {
		double q1 = 0.0;
		double q2 = 0.0;
		double	ret;

		if (NANorINF(x) && !INF(x)) 		/* x is a NaN */
			ret = x;
		else if (NANorINF(y) && !INF(y))	/* y is a NaN */
			ret = y;
		else {
			/* raise exception */
			/* x inf or y == 0 */
			q1 /= q2;
			MKNAN(ret);
		}
		return _domain_err(x,y,ret,"remainder",9);

	} else if (!x || NANorINF(y))
		return x;

	xexp = (short)EXPONENT(x);
	yexp = (short)EXPONENT(y);
	sign = (short)SIGNBIT(x);

	/* subnormal number */
	mask = fpsetmask(0);  /* mask all exceptions */
	nx = 0;
	if (yexp == 0) {
		t = 1.0, EXPONENT(t) += 57;
		y *= t; 
		nx = 57;
		yexp = (short)EXPONENT(y);
	}

	/* if y is tiny (biased exponent <= 57), scale up y to y*2**57 */
	else if (yexp <= (unsigned)57) {
		EXPONENT(y) += 57; 
		nx += 57; 
		yexp += 57;
	}

	nf = nx;
	SIGNBIT(x) = 0;
	SIGNBIT(y) = 0;
	/* mask off the least significant 27 bits of y */
	t = y; 
	LOFRACTION(t) &= (unsigned)0xf8000000;
	y2 = t;

	/* LOOP: argument reduction on x whenever x > y */
loop:
	while ( x > y ) {
		t = y;
		t1 = y2;
		xexp = (short)EXPONENT(x);
		k = xexp - yexp - (short)25;
		if (k > 0) 	/* if x/y >= 2**26, scale up y so that x/y < 2**26 */ {
			EXPONENT(t) += k;
			EXPONENT(t1) += k;
		}
		n = x / t; 
		x = (x - n * t1) - n * (t - t1);
	}
	/* end while (x > y) */

	if (nx != 0) {
		t = 1.0; 
		EXPONENT(t) += nx; 
		x *= t; 
		nx = 0; 
		goto loop;
	}

	/* final adjustment */

	hy = y / 2.0;
	if (x > hy || ((x == hy) && n % 2 == 1)) 
		x -= y;
	if (x == 0.0)
		SIGNBIT(x) = sign;
	else
		SIGNBIT(x) ^= sign;
	if (nf != 0) { 
		t = 1.0; 
		EXPONENT(t) -= nf; 
		x *= t;
	}
	(void)fpsetmask(mask);  /* reset exception masks */
	return(x);
}
#else
/* non-IEEE architectures */

#define MINEXP	DMINEXP

double	remainder(x, y)
register double	x, y;
{
	short	sign, mode = 0;
	double	hy, dy, b;
	extern 	int write();
	int	k, i = 1, yexp, dyexp;
	register double  yfr;

	if (y == 0.0) {
		struct exception exc;
		exc.type = DOMAIN;
		exc.name = "remainder";
		exc.arg1 = x;
		exc.arg2 = y;
		exc.retval = 0.0;
		if (_lib_version != c_issue_4)
			errno = EDOM;
		else if (!matherr(&exc)) {
			(void)write(2,"remainder: DOMAIN error\n",24);
			errno = EDOM;
		}
		return exc.retval;
	}
	y = _ABS(y);	
	yexp = (int)logb(y);
	if (yexp <= MINEXP ) {/* subnormal p, or almost subnormal p */
		b = ldexp(1.0, (int)DSIGNIF+1);
		y *= b; 
		mode = i = 2;
	}
	else if ( y >= MAXDOUBLE / 2) { 
		y /= 2 ; 
		x /= 2; 
		mode = 1;
	}
	dy = y + y; 
	hy = y / 2;
	yfr = frexp(dy, &dyexp);

	while (i--) {  /* do twice for de-normal y, else once */
		if (x < 0.0) {
			sign = 1;
			x = -x;
		}
		else sign = 0;
		while (x >= dy) {
			int xexp;
			double xfr = frexp(x, &xexp);
			x -= ldexp(dy, xexp - dyexp - (xfr < yfr));
		}
		if ( x > hy ) { 
			x -= y ;  
			if ( x >= hy ) 
				x -= y ; 
		}
		if (sign)
			x = -x;
		if (mode == 2 && i)
			x *= b;
	}
	if (mode == 1)
		x *= 2.0;
	else if (mode == 2)
		x /= b;
	return x;
}
#endif /* not IEEE */

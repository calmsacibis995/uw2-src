/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/sqrt.c	1.13"
/*LINTLIBRARY*/
/* 
 */

/* Two versions of the sqrt function are provided:
 * the first works only on IEEE architectures, the second is
 * more portable.  Both return the same result except that
 * the first rounds the last bit correctly as per the IEEE
 * rounding mode
 */

/* Special cases:
 * If x is a NaN, returns error EDOM and value NaN.  If x is also a
 * signalling NaN, raise an invalid op exception.
 * 
 * In -Xt mode,
 * if x < 0, raise an invalid op exception, return error EDOM and value 0.
 * 
 * In -Xa and -Xc modes,
 * if x < 0, raise an invalid op exception, return error EDOM and value NaN.
 */

#include "synonyms.h"
#include <math.h>
#include <errno.h>
#include <values.h>

#ifndef __STDC__
#define const
#endif

#if _IEEE /* for IEEE machines only */

#define P754_NOFAULT	1	/* avoid generating extra code */
#include <ieeefp.h>

#include "fpparts.h"

#define M_2_54	18014398509481984.0  /*  2**54  */

static const unsigned long	table[] = {
	0, 1204, 3062, 5746, 9193, 13348, 18162, 23592, 29598, 
	36145, 43202, 50740, 58733, 67158, 75992, 85215, 83599, 
	71378, 60428, 50647, 41945, 34246, 27478, 21581, 16499, 
	12183, 8588, 5674, 3403, 1742, 661, 130 
};

double	sqrt(x)
double	x;
{
	double	y, t;
	register double z;
	unsigned int	mx; 
	fp_rnd	r;
	register fp_except j;
	register int scalx = 0;

	if (NANorINF(x)) {
		if (INF(x)) {
			if (x>0)	/* x == +inf */
				return x;
			/* x == -inf - fall through to x <=0 check below */
		} else			/* x == NaN */
			return _domain_err(x,0.0,x,"sqrt",4);
	}

	if (x <= 0.0) {
		double q1=0.0;
		double q2=0.0;
		double ret=x;
		if (!x)		/* x == 0 */
			return x;

		/* raise invalid op exception */
		q1 /= q2;
		if (_lib_version == c_issue_4)
			ret = 0.0;
		else
			MKNAN(ret);

		return _domain_err(x,0.0,ret,"sqrt",4);
	}

	mx = EXPONENT(x);

	r = fpsetround(FP_RN); /* save rounding mode and set to default */
	j = fpsetsticky(0);  /* save and reset sticky bits */
	/* subnormal number, scale up x to x*2**54 */
	if (mx == 0) {
		x *= M_2_54;
		scalx = -27;
	}
	/* scale x to avoid intermediate over/underflow:
	 * if (x > 2**512) x=x/2**512; if (x < 2**-512) x=x*2**512 
	 */
	if (mx > 1535) {  /* 512 + 1023 */
		EXPONENT(x) -= 512;
		scalx += 256;
	}
	if (mx < 511) {  /* 1023 - 512 */
		EXPONENT(x) += 512;
		scalx -= 256;
	}
	/* magic initial approximation to almost 8 sig. bits */
	HIWORD(y) = ((int)HIWORD(x) >> 1) + 0x1ff80000;
	HIWORD(y) -= table[((int)(HIWORD(y)) >> 15) & 31];

	/* Heron's rule once with correction to improve y to 
	 * almost 18 sig. bits 
	 */
	t = x / y; 
	y +=  t; 
	HIWORD(y) -= 0x00100006;
	LOFRACTION(y) = 0;
	/* triple to almost 56 sig. bits; now y approx. sqrt(x) to within 1 ulp */
	t = y * y; 
	z = t;  
	EXPONENT(t) += 0x1;
	t += z; 
	z = (x - z) * y;
	t = z / (t + x);  
	EXPONENT(t) += 0x1;

	y += t;

	/* twiddle last bit to force y correctly rounded */
	(void)fpsetsticky(0); /* clear sticky bits */
	(void)fpsetround(FP_RZ);
	t = x/y;          /* ...chopped quotient, possibly inexact */
	j = fpsetsticky(j) & FP_X_IMP; /* was division exact? */
	if (j == 0) { 
		if (t == y)
			goto end;
		z = M_2_54 + 0.1; /* raise inexect flag */
		if (r == FP_RM || r == FP_RZ) {
			/* t = nextafter(t, NINF);    ...t=t-ulp */
			if (HIWORD(t) != 0)
				LOFRACTION(t) -= 0x1;
			else {
				LOWORD(t) = (unsigned)0xffffffff;
				if ((HIWORD(t) & 0x7fffffff) != 0)
					HIWORD(t) -= 0x1;
			}
		}
	}
	else {	/* j == 1 */
		z = M_2_54 + 0.1; /* raise inexect flag */
		if (r == FP_RN || r == FP_RP) {
			/* t = nextafter(t, PINF);    ...t=t+ulp */
			if (LOFRACTION(t) != (unsigned)0xffffffff)
				LOFRACTION(t) += 0x1;
			else {
				LOFRACTION(t) = 0;
				if ((HIWORD(t) & 0x7fffffff) != 0x7ff00000)
					HIWORD(t) += 0x1;
			}
		}
	}
	if (r == FP_RP) {
		/* y = nextafter(y, PINF);    ...y=y+ulp */
		if (LOFRACTION(y) != (unsigned)0xffffffff)
			LOFRACTION(y) += 0x1;
		else {
			LOFRACTION(y) = 0;
			if ((HIWORD(y) & 0x7fffffff) != 0x7ff00000)
				HIWORD(y) += 0x1;
		}
	}
	y += t;                          /* ...chopped sum */
	EXPONENT(y) -= 1;	/* correctly rounded sqrt */
end:
	EXPONENT(y) += scalx;	/* ...scale back y */
	(void)fpsetround(r);
	return(y);
}
#else	/* non-IEEE machines */

#define ITERATIONS	4

double
sqrt(x)
register double x;
{
	register double y;
	int iexp; /* can't be in register because of frexp() below */
	register int i = ITERATIONS;

	if (x <= 0) {

		if (x == 0.0)
			return (x); /* sqrt(0) == 0 */
		return _domain_err(x,0.0,0.0,"sqrt",4);
	}
	y = frexp(x, &iexp); /* 0.5 <= y < 1 */
	if (iexp % 2) { /* iexp is odd */
		--iexp;
		y += y; /* 1 <= y < 2 */
	}
	y = ldexp(y + 1.0, iexp/2 - 1); /* first guess for sqrt */
	do {
		y = 0.5 * (y + x/y);
	} while (--i > 0);
	return (y);
}
#endif

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/sqrtf.c	1.10"
/*LINTLIBRARY*/
/*
 *	Single Precision Routine - float args and float return.
 *	sqrt returns the square root of its single-precision argument,
 *	using Newton's method.
 *
 *	If on an IEEE machine, make sure last bit is rounded correctly
 *	according to the currently set rounding mode.
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
#include <errno.h>
#include <math.h>
#include <values.h>

#if _IEEE /* for IEEE machines only */
#define P754_NOFAULT	1	/* avoid generating extra code */
#include <ieeefp.h>
#include "fpparts.h"
#endif

#define ITERATIONS	4
#define M_2_54	18014398509481984.0  /*  2**54  */

float
sqrtf(float x)
{
	float y, t;
	int iexp;
	double z;
	register int i = ITERATIONS;
#if _IEEE
/* used to store rounding modes and sticky bits on IEEE machines */
	fp_rnd	r;
	register fp_except j;
	extern fp_rnd _fpsetround();
	extern fp_except _fpsetsticky();
#endif

	static float zero = 0.0;
	static float half = 0.5;

	if (FNANorINF(x)) {
		if (FINF(x)) {
			if (x>0)	/* x == +inf */
				return x;
			/* x == -inf - fall through to x <=0 check below */
		} else			/* x == NaN */
			return _float_domain(x,0.0F,x,"sqrtf",5);
	}

	if (x <= 0.0F) {
		float ret;
		if (!x)		/* x == 0 */
			return x;
#if _IEEE
		/* raise invalid op exception */
		zero /= zero;
#endif

		if (_lib_version == c_issue_4)
			ret = 0.0F;
		else
			FMKNAN(ret);

		return _float_domain(x,0.0F,ret,"sqrtf",5);
	}
#if _IEEE
	r = _fpsetround(FP_RN);  /* set round to nearest */
	j = _fpsetsticky(0);	/* clear sticky bits */
#endif
	y = frexp((double)x, &iexp); /* 0.5 <= y < 1 */
	if (iexp % 2) { /* iexp is odd */
		--iexp;
		y += y; /* 1 <= y < 2 */
	}
	y = ldexp((double)y + 1.0, iexp/2 - 1); /* first guess for sqrt */
	do {
		y = half * (y + x/y);
	} while (--i > 0);
#if _IEEE
	/* twiddle last bit to force y correctly rounded 
	 * on IEEE architectures
	 */
	(void)_fpsetround(FP_RZ); /* round toward zero */
	(void)_fpsetsticky(0);	/* clear sticky bits */
	t = x/y;          /* ...chopped quotient, possibly inexact */
	j = _fpsetsticky(j) & FP_X_IMP; /* was division exact? */
	if (j == 0) { 
		if (t == y)
			goto end;
		z = M_2_54 + 0.1; /* raise inexact flag */
		if (r == FP_RM || r == FP_RZ) {
			/* t = nextafter(t, NINF);    ...t=t-ulp */
			if ((FWORD(t) & 0x7fffffff) != 0)
				FWORD(t) -= 0x1;
		}
	}
	else {	/* j == 1 */
		z = M_2_54 + 0.1; /* raise inexact flag */
		if (r == FP_RN || r == FP_RP) {
			/* t = nextafter(t, PINF);    ...t=t+ulp */
			if ((FWORD(t) & 0x7fffffff) != 0x7f800000)
				FWORD(t) += 0x1;
		}
	}
	if (r == FP_RP) {
		/* y = nextafter(y, PINF);    ...y=y+ulp */
		if ((FWORD(y) & 0x7fffffff) != 0x7f800000)
			FWORD(y) += 0x1;
	}
	y += t;                          /* ...chopped sum */
	FEXPONENT(y) -= 1;	/* correctly rounded sqrt */
end:
	(void)_fpsetround(r); /* restore user rounding mode */
#endif
	return (y);
}

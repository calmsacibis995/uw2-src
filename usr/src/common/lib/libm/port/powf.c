/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/powf.c	1.10"
/*LINTLIBRARY*/
/*
 *	Single precision power function.
 *	powf(x, y) returns x ** y.
 *	uses double precision log and exp to preserve accuracy
 */
/*
 * Error cases:
 * 1. If x or y are NaN, returns error EDOM and value NaN.  If x or y are
 *    also signalling NaNs, raise an invalid op exception.
 * 2. If x is +-1 and y is +-inf, raise an invalid op exception and return
 *    error EDOM and value NaN.
 * 3. On underflow, return error ERANGE and value 0.
 *
 * In -Xt mode,
 * 4. On overflow, return error ERANGE and value +-HUGE.
 * 5. If x is +-0 and y < 0 raise a divide by zero exception, return error
 *    EDOM and value 0.
 * 6. If x < 0 and finite and y is finite and not an integer, raise an
 *    invalid op exception, return error EDOM and value 0.
 *
 * In -Xa and -Xc modes,
 * 4. On overflow, return error ERANGE and value +-HUGE_VAL.
 * 5. If x is +-0 and y < 0 and an odd integer raise a divide by zero 
 *    exception, return error EDOM and value +-HUGE_VAL.
 * 6. If x is +-0 and y < 0 and not an odd integer raise a divide by zero 
 *    exception, return error EDOM and value HUGE_VAL.
 * 7. If x < 0 and finite and y is finite and not an integer, raise an
 *    invalid op exception, return error EDOM and value NaN.
 *
 */

#include "synonyms.h"
#include <math.h>
#include <values.h>
#include <errno.h>
#include "fpparts.h"

/*
 * set the var odi if X is an odd integer.
 */
#define	ODI(X) { 					\
		if (X >= -MAXLONG && X <= MAXLONG) {	\
			k = (long) X;			\
			odi = ((float)k == X) && k % 2;\
		} else {				\
			t = fmodf(X,2.0F);		\
			odi = (t == 1.0F || t == -1.0F);	\
		}					\
	}

/* codes for error types */
#define OVER 1
#define UNDER 2
#define YooX1 3
#define X0Ylt0 4
#define Xlt0Yni 5

static float pow_exc(float, float, int, int);

float powf(float x, float y)
{
	register double a, b;
	int neg = 0;
	long k;
	int odi = 0;
	float t;

	static const float one = 1.0,
		     half = 0.5,
		     zero = 0.0;

	if (FNANorINF(y)) {
		if (!FINF(y))	/* y is NaN */
			return _float_domain(x,y,y,"powf",4);
		else {		/* y is inf */
			float	z;
			int	exc_type;
			if (FNANorINF(x) && !FINF(x))	/* x is NaN */
				return _float_domain(x,y,x,"powf",4);
			z = _ABS(x);
			if (z > 1.0F) {
				if (y > 0.0F)
					exc_type = OVER;
				else
					exc_type = UNDER;
			}
			else if (z==1.0F)
				exc_type = YooX1;
			else 
				exc_type = (y < 0.0F) ? OVER : UNDER;
			return pow_exc(x, y, exc_type, 0);
		}
	}
	if (FNANorINF(x)) {
		if (!y)
			return 1.0F;
		if (!FINF(x))	/* x is NaN */
			return _float_domain(x,y,x,"powf",4);
		else {		/* x is inf */
			int	exc_type;
			if (x > 0.0F) {	/* +inf */
				exc_type = (y > 0.0F) ? OVER : UNDER;
			} else {		/* -inf */
				ODI(y);
				neg = odi;
				if (y>0) 
					exc_type = OVER;
				else
					exc_type = UNDER;
			}
			return pow_exc(x, y, exc_type, neg);
		}
	}
	if (!y) {
		if (_lib_version == c_issue_4 && !x)
			return _float_domain(x, y, 0.0F, "powf", 4);
		return 1.0F;
	}
	if (!x) {
		ODI(y);
		if (y>0.0F) 
			return(odi ? x : +0.0F);
		else {
			return(pow_exc(x,y,X0Ylt0,odi)); 
		}
	}

	a = (double)x;
	b = (double)y;
	neg = 0;
	if (x < zero) { /* test using integer arithmetic if possible */
		if (y >= -FMAXPOWTWO && y <= FMAXPOWTWO) {
			register long ly = (long)y;

			if ((float)ly != y)
				return(pow_exc(x, y, Xlt0Yni, 0)); 
				/* y not integral */
			neg = ly % 2;
		} else { /* y must be an integer */
			float  dum;

			if (modff(y * half, &dum))
				neg++; /* y is odd */
			else
				return(pow_exc(x, y, Xlt0Yni, 0)); 
		}
		a = -a;
	}
	if (a != 1.0) { /* x isn't the final result */
		/* the following code protects against multiplying x and y
		 * until there is no chance of multiplicative overflow */
		if ((a = log(a)) < 0) { /* preserve sign of product */
			a = -a;
			b = -b;
		}
		if (b > (double)LN_MAXFLOAT/a) {
			return(pow_exc(x, y, OVER, neg));
		}
		if (b < (double)LN_MINFLOAT/a) {
			return(pow_exc(x, y, UNDER, neg));
		}
		a = exp(a * b); /* finally; no mishap can occur */
	}
	return (float)(neg ? -a : a);
}

static float
pow_exc(float x, float y, int etype, int neg)
{
	struct exception exc;
	int err;
	float q1=0.0F;
	float q2=0.0F;
	float ret;

	exc.type = DOMAIN;
	err = EDOM;

	switch(etype) {
	case YooX1:	/* y is inf, X is +-1 */
#if _IEEE
		q1 /= q2;		/* raise invalid op exception */
#endif
		FMKNAN(ret);
		break;
	case X0Ylt0:	/* x is 0, y < 0 (if y is an odd int, neg is set) */
#if _IEEE
		q1 = 1.0F / q1;		/* raise divide-by-zero exception */
#endif
		if (_lib_version == c_issue_4) 
			ret = 0.0F;
		else {
			if (neg)
				ret = FSIGNBIT(x) ? 
					(float)-HUGE_VAL : 
					(float)HUGE_VAL;
			else
				ret = (float)HUGE_VAL;
		}
		break;
	case Xlt0Yni:	/* x < 0, y is not a integer */
#if _IEEE
		q1 /= q2;		/* raise invalid op exception */
#endif
		if (_lib_version == c_issue_4) 
			ret = 0.0F;
		else 
			FMKNAN(ret);
		break;
	case UNDER:	/* underflow occured */
		err = ERANGE;
		exc.type = UNDERFLOW;
		ret = neg ? -0.0F : 0.0F;
		break;
	case OVER:	/* overflow occured */
		err = ERANGE;
		exc.type = OVERFLOW;
		if (_lib_version == c_issue_4) 
			ret = (neg ? (float)-HUGE: (float)HUGE);
		else 
			ret = (neg ? (float)-HUGE_VAL: (float)HUGE_VAL);
		break;
	}

	if (_lib_version != c_issue_4) {
		errno = err;
		return ret;
	}
	else {
		exc.arg1 = x;
		exc.arg2 = y;
		exc.name = "powf";
		exc.retval = (double)ret;
		if (!matherr(&exc)) {
			if (err == EDOM)
				(void)write(2,"powf: DOMAIN error\n",19);

			errno = err;
		}
		return (float)exc.retval;
	}
}

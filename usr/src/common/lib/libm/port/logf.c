/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/logf.c	1.9"
/*LINTLIBRARY*/
/*
 *
 *	Single Precision Log
 *	logf returns the natural logarithm of its single-precision argument.
 *	log10f returns the base-10 logarithm of its single-precision argument.
 *	If argument is NaN, return EDOM error and value NaN.  If argument is
 *	also a signalling NaN, raise the invalid op exception.
 *
 *	In -Xt mode,
 *	If x < 0 raise the invalid op exception and return error EDOM and
 *	value -HUGE.
 *	If x = 0 raise the divide by zero exception and return error EDOM
 *	and value -HUGE.
 *
 *	In -Xa and -Xc modes,
 *	If x < 0 raise the invalid op exception and return error EDOM and
 *	value NaN.
 *	If x = 0 raise the divide by zero exception and return error ERANGE
 *	and value -HUGE_VAL.
 * Method :
 *	1. Argument Reduction: find k and f such that 
 *			x = 2^k * (1+f), 
 *	   where  sqrt(2)/2 < 1+f < sqrt(2) .
 *
 *	2. Let s = f/(2+f) ; based on log(1+f) = log(1+s) - log(1-s)
 *		 = 2s + 2/3 s**3 + 2/5 s**5 + .....,
 *	   log(1+f) is computed by
 *
 *	     		log(1+f) = 2s + s*log__L(s*s)
 *	   where
 *		log__L(z) = z*(L1 + z*(L2 + z*(... (L6 + z*L7)...)))
 *
 *
 *	3. Finally,  log(x) = n*ln2 + log(1+f).  (Here n*ln2 will be stored
 *	   in two floating point number: n*ln2hi + n*ln2lo, n*ln2hi is exact
 *	   since the last 8 bits of ln2hi is 0.)
 */

#include "synonyms.h"
#include <math.h>
#include <errno.h>
#include <values.h>
#include "fpparts.h"

static float log_error(float, const char *, int);


/* coefficients for polynomial expansion  (log__L())*/
static const float p[] = {
	2.2222198607186277597E-1,
	2.8571428742008753154E-1,   
	3.9999999999416702146E-1,    
	6.6666666666667340202E-1
};
static const float
ln2hi = 6.9314575195e-1,
ln2lo = 1.4286067653e-6;

static const float zero = 0.0,
	     half = 0.5,
	     two = 2.0,
	     negone = -1.0;

float
logf(float x)
{
	float z, t, s;
	int n, k;

	if (FNANorINF(x)) {
		if (FINF(x))	/* x is inf */
			return(FSIGNBIT(x) ? log_error(x,"logf",4) : x);
		else 		/* x is NaN */
			return _float_domain(x,0.0F,x,"logf",4);
	} 
	if (x <= zero)
		return(log_error(x,"logf",4));

#if _IEEE
	/* inline expansion of logb()  - get unbiased exponent of x*/
	if ((n = FEXPONENT(x)) == 0) /* de-normal */
		n = -126;
	else n -= 127;
	if (n != -126)  /*  in-line expand ldexp if not de-normal */
		FEXPONENT(x) -= n;	
	else  
		x = (float)ldexp((double)x, -n);
#else
	n = (int)logb((double)x);
	x = (float)ldexp((double)x, -n);
#endif
	if ( n == -126) { /* sub-normal */
#if _IEEE
		/* inline expansion of logb() */
		k = FEXPONENT(x) - 127; /* can't be subnormal because of
				         * prior ldexp
				         */
		FEXPONENT(x) -= k;	
#else
		k = (int)logb((double)x);
		x = (float)ldexp((double)x, -k);
#endif
		n += k;
	}
        if (x >= (float)M_SQRT2 ) {
		n += 1;
		x *= half;
	}
	x += negone;

	/* compute log(1+x)  */
	s = x / (two + x);
	t = x * x * half;
	z = s * s;
	z = n * ln2lo + s * (t + _POLY3(z, p) * z);
	x += (z - t) ;
	return(n * ln2hi + x);
}

float
log10f(float x)
{
	if (FNANorINF(x)) {
		if (FINF(x))	/* x is inf */
			return(FSIGNBIT(x) ? log_error(x,"log10f",6) : x);
		else 		/* x is NaN */
			return _float_domain(x,0.0F,x,"log10f",6);
	} 
	if (x <= zero)
		return(log_error(x,"log10f",6));
	return (logf(x) * M_LOG10E);
}

static float
log_error(float x, const char *f_name, int name_len)
{
	int zflag = 0;
	float ret;
	float q1=0.0F;
	float q2=0.0F;


	if (!x) {
#if _IEEE
		/* raise divide-by-zero exception */
		q1 = 1.0F / q1;
#endif
		zflag = 1;
		ret = (float)-HUGE_VAL;
	} else {		/* x < 0 */
		/* raise invalid op exception */
#if _IEEE
		q1 /= q2;
#endif
		FMKNAN(ret);
	}

	if (_lib_version != c_issue_4) {
		errno = zflag ? ERANGE : EDOM;
		return ret;
	}
	else {
		struct exception exc;
		exc.arg1 = (float)x;
		exc.name = (char *)f_name;
		exc.retval = -HUGE;
		exc.type = zflag ? SING : DOMAIN;

		if (!matherr(&exc)) {
			(void) write(2, f_name, name_len);
			if (zflag)
				(void) write(2,": SING error\n",13);
			else
				(void) write(2,": DOMAIN error\n",15);
			errno = EDOM;
		}
		return ((float)exc.retval);
	}
}

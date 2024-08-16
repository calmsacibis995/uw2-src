/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/log.c	1.10"
/*LINTLIBRARY*/
/*
 *
 *	log returns the natural logarithm of its double-precision argument.
 *	log10 returns the base-10 logarithm of its double-precision argument.
 *
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
 *
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
 *	   since the last 20 bits of ln2hi is 0.)
 */

#include "synonyms.h"
#include <math.h>
#include <errno.h>
#include <values.h>
#include "fpparts.h"

#ifndef __STDC__
#define const
#endif

extern int write();
static double log_error();

#if _IEEE
static const double
ln2hi  =  6.9314718036912381649E-1    ,
ln2lo  =  1.9082149292705877000E-10   ;
#endif

/* coefficients for polynomial expansion  (log__L())*/
#if _IEEE
static const double p[] = {
	1.4795612545334174692E-1,
	1.5314087275331442206E-1,
	1.8183562745289935658E-1, 
	2.2222198607186277597E-1,
	2.8571428742008753154E-1,   
	3.9999999999416702146E-1,    
	6.6666666666667340202E-1,
	0.0
};
#endif

double
log(x)
double x;
{
	register double x1, z, t;
	double s;
	int n, k;

	if (NANorINF(x)) {
		if (INF(x))	
			return((x < 0) ? log_error(x,"log",3) : x);
		else 		/* x is NaN */
			return _domain_err(x,0.0,x,"log",3);
	} else if (x <= 0.0)
		return(log_error(x,"log",3));
			
#if _IEEE
	/* inline expansion of logb()  - get unbiased exponent of x*/
	if ((n = EXPONENT(x)) == 0) /* de-normal */
		n = -1022;
	else n -= 1023;
	if (n != -1022)  /*  in-line expand ldexp if not de-normal */
		EXPONENT(x) -= n;	
	else  
		x = ldexp(x, -n);
#else
	n = (int)logb(x);
	x = ldexp(x, -n);
#endif
	if ( n == -1022) { /* sub-normal */
#if _IEEE
		/* inline expansion of logb() */
		k = EXPONENT(x) - 1023; /* can't be subnormal because of
				         * prior ldexp
				         */
		EXPONENT(x) -= k;	
#else
		k = (int)logb(x);
		x = ldexp(x, -k);
#endif
		n += k;
	}
	x1 = x; /* x can't be in register because of inline expansion
		 * above
		 */
        if (x1 >= M_SQRT2 ) {
		n += 1;
		x1 *= 0.5;
	}
	x1 += -1.0;

	/* compute log(1+x)  */
	s = x1 / (2 + x1);
	t = x1 * x1 * 0.5;
	z = s * s;
#if _IEEE
	z = n * ln2lo + s * (t + _POLY7(z, p));
#endif
	x1 += (z - t) ;
	return(n * ln2hi + x1);
}

double
log10(x)
double x;
{
	if (NANorINF(x)) {
		if (INF(x))	
			return((x<0) ? log_error(x,"log10",5) : x);
		else 		/* x is NaN */
			return _domain_err(x,0.0,x,"log10",5);
	} else if (x <= 0.0)
		return(log_error(x,"log10",5));
	
	return(log(x) * M_LOG10E);
}

static double
log_error(x, f_name, name_len)
double x;
char *f_name;
unsigned int name_len;
{
	register int zflag = 0;
	struct exception exc;
	double q1=0.0;
	double q2=0.0;
	int err=EDOM;

	exc.arg1 = x;
	exc.name = f_name;

	if (!x) {		/* x == 0 */
		/* raise divide-by-zero exception */
#ifdef _IEEE
		q1 = 1.0 / q1;
#endif
		exc.retval = -HUGE_VAL;
		err=ERANGE;
		zflag = 1;
	} else {		/* x < 0 */
		/* raise invalid op exception */
#ifdef _IEEE
		q1 /= q2;
#endif
		MKNAN(exc.retval);	/* make a NaN for the return value */
	}

	if (_lib_version != c_issue_4)
		errno = err;
	else {
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
	}
	return (exc.retval);
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/pow.c	1.18"
/*LINTLIBRARY*/
/* 
 */

/* POW(X,Y)  
 * RETURN X**Y 
 *
 * Note - if y is 0, 1 ise *always* returned.
 *
 * Method
 *	1. Compute and return log(x) in three pieces:
 *		log(x) = n*ln2 + hi + lo,
 *	   where n is an integer.
 *	2. Perform y*log(x) by simulating muti-precision arithmetic and 
 *	   return the answer in three pieces:
 *		y*log(x) = m*ln2 + hi + lo,
 *	   where m is an integer.
 *	3. Return x**y = exp(y*log(x))
 *		= 2^m * ( exp(hi+lo) ).
 *
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

#ifndef __STDC__
#define const
#endif

#include "synonyms.h"
#include <math.h>
#include <values.h>
#include <errno.h>
#include "fpparts.h"

extern int write();

#if _IEEE
static const double
ln2hi  =  6.9314718036912381649E-1,
ln2lo  =  1.9082149292705877000E-10,
invln2 =  1.4426950408889633870E0,
sqrt2  =  1.4142135623730951455E0;
#endif

/* coefficients for polynomial expansion */
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

/* codes for error types */
#define OVER 1
#define UNDER 2
#define YinfX1 3
#define X0Ylt0 4
#define Xlt0Yni 5

static double pow_exc(),_exp__E();

double	pow(x, y)
double	x, y;
{
	register double s, c, t;
	double	z, tx, ty;
	float	sx, sy;
	long	k = 0;
	int	n, m, r;
	int 	neg_rslt = 0;
	int	xneg = 0;
	int	saverrno;
	int	odi;

	if (NANorINF(y)) {
		if (!INF(y))	/* y is NaN */
			return _domain_err(x,y,y,"pow",3);
		else {		/* y is inf */
			int	exc_type;

			if (NANorINF(x) && !INF(x))	/* x is NaN */
				return _domain_err(x,y,x,"pow",3);
			z = _ABS(x);
			if (z > 1.0) {
				if (y > 0.0)
					exc_type = OVER;
				else
					exc_type = UNDER;
			}
			else if (z==1.0)
				exc_type = YinfX1;
			else 
				exc_type = (y < 0) ? OVER : UNDER;
			return pow_exc(x, y, exc_type, 0);
		}
	} else if (NANorINF(x)) {

		if (!y)
			return 1.0;
		if (!INF(x))	/* x is NaN */
			return _domain_err(x,y,x,"pow",3);
		else {		/* x is inf */
			int exc_type;
			if (x > 0) {	/* +inf */
				exc_type = (y > 0) ? OVER : UNDER;
			}
			else {		/* -inf */

				/* is y an odd integer? */
				if (y >= -MAXLONG && y <= MAXLONG) {
					k = (long) y;			
					odi = ((double)k == y) && k % 2;
				} else {				
					t = fmod(y,2.0);		
					odi = (t == 1.0 || t == -1.0);	
				}					
				neg_rslt = odi;
				if (y>0) 
					exc_type = OVER;
				else
					exc_type = UNDER;
			}
			return pow_exc(x, y, exc_type, neg_rslt);
		}
	} else if (!y) {
		if (_lib_version == c_issue_4 && !x)
			return _domain_err(x, y, 0.0, "pow", 3);
		return (1.0);
	} else if (!x) {
		/* is y an odd integer? */
		if (y >= -MAXLONG && y <= MAXLONG) {
			k = (long) y;			
			odi = ((double)k == y) && k % 2;
		} else {				
			t = fmod(y,2.0);		
			odi = (t == 1.0 || t == -1.0);	
		}					
		if (y>0) 
			return(odi ? x : +0.0);
		else {
			return(pow_exc(x,y,X0Ylt0,odi)); 
		}
	}

	if (y == 1.0 || x == 1.0)
		return x;

	if (x < 0.0) {
		x = -x;
		xneg = 1;
		if (y >= -MAXLONG && y <= MAXLONG) {
			k = (long) y;
			if ((double)k != y) /* y not integral */
				return(pow_exc(-x, y, Xlt0Yni, 0)); 
			neg_rslt = k % 2;
		}
		else {
			if (!((t = fmod(y, 2.0)) == 0.0 || t == 1.0 || t == -1.0))
				return(pow_exc(-x, y, Xlt0Yni, 0)); 
			if (t == 1.0 || t == -1.0)
				neg_rslt++; /* y is an odd integer */
		}
	}
	/* reduce x to z in [sqrt(1/2)-1, sqrt(2)-1] */
#if _IEEE
	/* inline expand logb() for efficiency */
	if ((n = EXPONENT(x)) == 0)
		n = -1022;
	else n -= 1023;
	if (n > -1022) {
	/* inline expand ldexp for efficiency */
		z = x;
		EXPONENT(z) -=n;
	}
	else {
		z = ldexp(x, -n);
		if ((m = EXPONENT(z)) == 0)
			m = -1022;
		else m -= 1023;
		n += m;
		z = ldexp(z, -m);
	}
#else
	z = ldexp(x, -(n = logb(x)));
#endif
	if (z >= sqrt2 ) {
		n += 1; 
		z *= 0.5;
	}  
	z -= 1.0;

	/* log(x) = nlog2+log(1+z) ~ nlog2 + t + tx */
	s = z / (2.0 + z); 
	c = z * z * 0.5; 
	t = s * s;
#if _IEEE
	tx = s * (c + _POLY7(t, p));
#endif
	t = z - (c - tx); 
	tx += (z - t) - c;

	/* if y*log(x) is neither too big nor too small */
#if _IEEE
	/* expand logb inline for efficiency */
	z = n + t;
	if ((m = EXPONENT(z)) == 0)
		m = -1022;
	else m -= 1023;
	if ((r = EXPONENT(y)) == 0)
		r = -1022;
	else r -= 1023;
	if ((s = m + r) < 12.0)
#else
	if ((s = logb(y) + logb(n + t)) < 12.0)
#endif
		if (s > -60.0) {

			/* compute y*log(x) ~ mlog2 + t + c */
			s = y * (n + invln2 * t);
			m = s + (s < 0.0 ? -0.5 : 0.5);
			k = (long)y;
			/* m := nint(y*log(x)) */

			if ((double)k == y) {	/* if y is an integer */
				k = m - k * n;
				sx = t; 
				tx += (t - sx); 
			} else {		/* if y is not an integer */
				k = m;
				tx += n * ln2lo;
				sx = (c = n * ln2hi) + t; 
				tx += (c - sx) + t; 
			}
			/* end of checking whether k==y */
			sy = y; 
			ty = y - sy;          /* y ~ sy + ty */
			s = (double)sx * sy - k * ln2hi;        /* (sy+ty)*(sx+tx)-kln2 */
			z = (tx * ty - k * ln2lo);
			tx = tx * sy; 
			ty = sx * ty;
			t = ty + z; 
			t += tx; 
			t += s;
			c = -((((t - s) - tx) - ty) - z);

			/* return exp(y*log(x)) */
			t += _exp__E(t, c); 
			saverrno = errno;
			errno = 0;
			t = ldexp(1.0 + t, m);
			if (errno == ERANGE) {
				return(pow_exc((xneg ? -x : x), y, (t ? OVER:
					UNDER), neg_rslt));
			}
			errno = saverrno;
			return(neg_rslt ? -t : t);
		}
	/* end of if log(y*log(x)) > -60.0 */

		else 
			return(neg_rslt ? -1.0 : 1.0);
	else if (((y < 0.0) ? -1.0 : 1.0) * (n + invln2 * t) < 0.0) {
		/* exp(-(big#)) underflows to zero */

		return(pow_exc(xneg ? -x : x, y, UNDER, 0));
	} else {
		/* exp(+(big#)) overflows to INF */
		return(pow_exc(xneg ? -x : x, y, OVER, neg_rslt ));
	}

}

/* _exp__E(x,c)
 * ASSUMPTION: c << x  SO THAT  fl(x+c)=x.
 * (c is the correction term for x)
 * exp__E RETURNS
 *
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
 *	   (See the listing below for the values of p1,q1. The poly-
 *	    nomials P and Q may be regarded as the approximations to sinh
 *	    and cosh :
 *		sinh(r/2) =  r/2 + r * P  ,  cosh(r/2) =  1 + Q . )
 *
 *         The coefficients were obtained by a special Remez algorithm.
 */

#define SMALL	1.0E-19

#if _IEEE
static double 
p1 = 1.3887401997267371720E-2,
p2 = 3.3044019718331897649E-5, 
q1 = 1.1110813732786649355E-1,
q2 = 9.9176615021572857300E-4;
#endif

static
double _exp__E(x,c)
double x, c;
{
	register double z, p3, q;
	double xp, xh, w;

	if (_ABS(x) > SMALL) {
           z = x * x;
	   p3 = z * (p1 + z * p2);
#if _IEEE
           q = z * (q1 + z * q2);
#endif
           xp= x * p3; 
	   xh= x * 0.5;
           w = xh - (q - xp);
	   p3 += p3;
	   c += x * ((xh * w - (q - (p3 + xp)))/(1.0 - w) + c);
	   return(z * 0.5 + c);
	}
	/* end of |x| > small */

	else 
	    return 0.0;
}

static double
pow_exc(x, y, etype, neg)
double x, y;
register int etype, neg;
{
	struct exception exc;
	int err;
	double z1=0.0;
	double z2=0.0;

	exc.arg1 = x;
	exc.arg2 = y;
	exc.name = "pow";
	exc.type = DOMAIN;
	err=EDOM;
	switch(etype) {
	case YinfX1:	/* y is inf, X is +-1 */
#ifdef _IEEE
		z1 /= z2;		/* raise invalid op exception */
#endif
		MKNAN(exc.retval);
		break;
	case X0Ylt0:	/* x is 0, y < 0 (if y is an odd int, neg is set) */
#ifdef _IEEE
		z1 = 1.0 / z1;		/* raise divide-by-zero exception */
#endif
		if (_lib_version == c_issue_4) 
			exc.retval = 0.0;
		else {
			if (neg)
				exc.retval = SIGNBIT(x) ? -HUGE_VAL : HUGE_VAL;
			else
				exc.retval = HUGE_VAL;
		}
		break;
	case Xlt0Yni:	/* x < 0, y is not a integer */
#ifdef _IEEE
		z1 /= z2;		/* raise invalid op exception */
#endif
		if (_lib_version == c_issue_4) 
			exc.retval = 0.0;
		else 
			MKNAN(exc.retval);
		break;
	case UNDER:	/* underflow occured */
		err=ERANGE;
		exc.type = UNDERFLOW;
		exc.retval = neg ? -0.0 : 0.0;
		break;
	case OVER:	/* overflow occured */
		err=ERANGE;
		exc.type = OVERFLOW;
		if (_lib_version == c_issue_4) 
			exc.retval = (neg ? -HUGE: HUGE);
		else 
			exc.retval = (neg ? -HUGE_VAL: HUGE_VAL);
		break;
	}

	if (_lib_version != c_issue_4)
		errno = err;
	else if (!matherr(&exc)) {
		if (err == EDOM)
			(void)write(2,"pow: DOMAIN error\n",18);

		errno=err;
	}
	return exc.retval;
}

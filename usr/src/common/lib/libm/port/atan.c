/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/atan.c	1.8"

 /*	atan returns the arctangent of its double-precision argument,
 *	in the range [-pi/2, pi/2].
 *	if x is a NaN, errno is set to EDOM and x returned.
 *
 *	atan2(y, x) returns the arctangent of y/x,
 *	in the range (-pi, pi].
 *	atan2 discovers what quadrant the angle is in and calls atan.
 *	If either argument to atan2 is NaN, atan2 returns EDOM error and
 *	value NaN.
 *
 *	In -Xt mode,  
 *	atan2 returns EDOM error and value 0 if both arguments are zero.
 *
 *	In -Xa and -Xc modes, 
 *	atan2 returns EDOM error and value +-0 	if y is +-0 and x is +0
 *	atan2 returns EDOM error and value +-pi if y is +-0 and x is -0
 *	
 */

/* atan(x)
 * Method :
 *	1. Reduce x to positive by atan(x)=-atan(-x).
 *	2. According to the integer k=4x+0.0625 truncated ,  the argument 
 *	   is further reduced to one of the following intervals and the 
 *	   arctangent of x is evaluated by the corresponding formula:
 *
 *         [0,7/16]	   atan(x) = x - x^3*(a1+x^2*(a2+...(a10+x^2*a11)...)
 *	   [7/16,11/16]    atan(x) = atan(1/2) + atan( (x-1/2)/(1+x/2) )
 *	   [11/16.19/16]   atan(x) = atan(1) + atan( (x-1)/(1+x) )
 *	   [19/16,39/16]   atan(x) = atan(3/2) + atan( (x-1.5)/(1.5x) )
 *	   [39/16,INF]     atan(x) = atan(INF) + atan( -1/x )
 */

#include "synonyms.h"
#include <errno.h>
#include <math.h>
#include <values.h>
#include <fpparts.h>

#ifndef __STDC__
#define const
#endif

extern int write();

#define _POLY10(x, c)	(_POLY9((x), (c)) * (x) + (c)[10])
#define _POLY11(x, c)	(_POLY10((x), (c)) * (x) + (c)[11])

#define M_PI_34		2.35619449019234492884

static const double atanhi[] = {
4.6364760900080609352E-1, /* atan(0.5) hi :  2^ -2 * Hex 1.DAC670561BB4F */
7.8539816339744827900E-1, /* atan(1.0) hi :  2^ -1 * Hex 1.921FB54442D18 */
9.8279372324732905408E-1, /* atan(1.5) hi :  2^ -1 * Hex 1.F730BD281F69B */
1.5707963267948965580E0 , /* atan(inf) hi :  2^  0 * Hex 1.921FB54442D18 */
};
static const double atanlo[] = {	
 2.2698777452961687092E-17, /* atan(0.5) lo : 2^-56 * Hex  1.A2B7F222F65E2 */
 3.0616169978683830180E-17, /* atan(1.0) lo : 2^-55 * Hex  1.1A62633145C07 */
 1.3903311031230998452E-17, /* atan(1.5) lo : 2^-56 * Hex  1.007887AF0CBBD */
 6.1232339957367660359E-17, /* atan(inf) lo : 2^-54 * Hex  1.1A62633145C07 */
};

static const double 	a[] = {
  1.6285820115365782362E-2    , /*a11 2^ -6  * Hex  1.0AD3AE322DA11 */
 -3.6531572744216915527E-2    , /*a10 2^ -5  * Hex -1.2B4442C6A6C2F */
  4.9768779946159323602E-2    , /*a9  2^ -5  * Hex  1.97B4B24760DEB */
 -5.8335701337905734865E-2    , /*a8  2^ -5  * Hex -1.DDE2D52DEFD9A */
  6.6610731373875312067E-2    , /*a7  2^ -4  * Hex  1.10D66A0D03D51 */
 -7.6918762050448299950E-2    , /*a6  2^ -4  * Hex -1.3B0F2AF749A6D */
  9.0908871334365065620E-2    , /*a5  2^ -4  * Hex  1.745CDC54C206E */
 -1.1111110405462355788E-1    , /*a4  2^ -4  * Hex -1.C71C6FE231671 */
  1.4285714272503466371E-1    , /*a3  2^ -3  * Hex  1.24924920083FF */
 -1.9999999999876483248E-1    , /*a2  2^ -3  * Hex -1.999999998EBC4 */
  3.3333333333332931803E-1    , /*a1  2^ -2  * Hex  1.555555555550D */
};

/*
 * Handle error returns.
 * Either both x and y are zero, or one or both are NaNs.
 */
static double
atan_err(x,y,num)
double x,y;
int num;		/* number of tans - atan == 1, atan2 == 2... */
{
	struct exception exc;

	/*
	 * If not quiet NaN, raise invalid op exception.
	 */

#ifdef _IEEE
	if (NANorINF(x) && !INF(x)) {	/* x is a NaN */
		exc.retval = x;
		SigNAN(exc.retval);
	} else if (NANorINF(y)) {	/* y is a NaN (can't be inf) */
		exc.retval = y;
		SigNAN(exc.retval);
	} else {			/* x == 0 and y == 0 */
		if (_lib_version == c_issue_4)
			exc.retval = 0.0;
		else if (SIGNBIT(x)) 		/* -0.0 */
			exc.retval = SIGNBIT(y) ? -M_PI : M_PI;
		else				/* + 0.0 */
			exc.retval = y;

	}
#else
	exc.retval = 0.0;
#endif

	if (_lib_version == c_issue_4) {
		exc.arg1 = y;
		exc.arg2 = x;
		exc.type = DOMAIN;
		exc.name = num == 2 ? "atan2" : "atan";
		if (!matherr(&exc)) {
			(void) write(2, exc.name, num == 2 ? 5 : 4);
			(void) write(2, ": DOMAIN error\n", 15);
			errno=EDOM;
		}
	} else
		errno=EDOM;
		
	return(exc.retval);
				
}


double atan(arg)
double  arg;
{  
	static const double small = 1.0E-9, big = 1.0E18;
	double  hi, lo;
	register double z;
	register double x;
	register int k, signx = 0;
	
	if (NANorINF(arg) && !INF(arg))
		return atan_err(0.0,arg,1);

	if (arg < 0.0) {
		signx = 1;
		x = -arg;
	} else
		x = arg;
		
	if (x < 2.4375) {		 
		/* truncate 4(x+1/16) to integer for branching */
		k = 4 * (x + 0.0625);
		switch (k) {
		    /* x is in [0,7/16] */
		case 0:                    
		case 1:
			hi = 0.0;
			lo = 0.0;			
			if (x < small) 
				return(signx ? -x : x);
			break;
		/* x is in [7/16,11/16] */
		case 2:                    
			hi = atanhi[0];
			lo = atanlo[0];
			x = ((x + x) - 1.0 ) / ( 2.0 +  x );
			break;
		/* x is in [11/16,19/16] */
		case 3:                    
		case 4:
			hi = atanhi[1];
			lo = atanlo[1];
			x = ( x - 1.0 ) / ( x + 1.0 );
			break;
		/* x is in [19/16,39/16] */
		default:                   
			hi = atanhi[2];
			lo = atanlo[2];
			z = x - 1.0;
			x = x + x + x;
			x = ((z + z) - 1.0 ) / ( 2.0 + x ); 
			break;
		} /* switch */
	}
	/* end of if (x < 2.4375) */
	else {                          
		if (x <= big) {
	    	/* x is in [2.4375, big] */
			x = - 1.0 / x;
			hi = atanhi[3];
			lo = atanlo[3];
	    	}
		else          
	    	/* x is in [big, INF] */
			return(signx ? -M_PI_2 : M_PI_2);
	}
    /* end of argument reduction */

    /* compute atan(x) for x in [-.4375, .4375] */
	z = x * x;

	z = x * z * _POLY10(z, a);

 	z = lo - z;
	z += x; 
	z += hi;
	return(signx ? -z : z);
}

double atan2(y, x)
double y, x;
{
	register int neg_y;
	double at, mod_y;

	if (NANorINF(y)) {
		if (!INF(y))	/* y is a NaN */
			return (atan_err(x,y,2));	
		else if (NANorINF(x)) {		/* y is inf */
			if (!INF(x))	/* x is a NaN */
				return (atan_err(x,y,2));
			else {		/* x is inf */
				if (x > 0) /* +inf */
					return(y>0 ? M_PI_4:-M_PI_4);
				else 	   /* -inf */
					return(y>0 ? M_PI_34: -M_PI_34);
			}
		} else 			/* x is finite */
			return (y>0 ? M_PI_2 : -M_PI_2);
	} else if (NANorINF(x)) { 
		if (!INF(x))	/* x is a NaN */
			return (atan_err(x,y,2));
		else if (y) { /* x is inf (and y is finite and non-zero) */
			if (x > 0) /* +inf */
				return (y>0 ? +0.0 : -0.0);
			else	   /* -inf */
				return (y>0 ? M_PI : -M_PI);
		}
	} else if (x ==0.0 && y == 0.0)
		return (atan_err(x,y,2));
			

	/*
	 * The next lines determine if |x| is negligible compared to |y|,
	 * without dividing, and without adding values of the same sign.
	 */
#if _IEEE
	if ((neg_y = SIGNBIT(y)) != 0)
#else
	if ((neg_y = ( y < 0)) != 0)
#endif
		mod_y = -y;
	else
		mod_y = y;

	if (mod_y - _ABS(x) == mod_y)
		return (neg_y ? -M_PI_2 : M_PI_2);
	/*
	 * The next line assumes that if y/x underflows the result
	 * is zero with no error indication, so it's safe to divide.
	 */
	at = atan(y/x);
	if (x > 0)		     /* return arctangent directly */
		return (at);
	if (neg_y)		    /* x < 0, adjust arctangent for */
		return (at - M_PI);        /* correct quadrant */
	else
		return (at + M_PI);
}

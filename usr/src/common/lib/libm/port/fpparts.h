/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/fpparts.h	1.5"

/* Macros to pull apart parts of single and  double precision
 * floating point numbers in IEEE format
 * Be sure to include /usr/include/values.h before including
 * this file to get the required definition of _IEEE
 */

#if _IEEE

#include "machfp.h"

/* Don't do fp optimisiations */
#pragma fenv_access on

/* parts of a double precision floating point number */
#define	SIGNBIT(X)	(((_dval *)&(X))->fparts.sign)
#define EXPONENT(X)	(((_dval *)&(X))->fparts.exp)
#define HIFRACTION(X)	(((_dval *)&(X))->fparts.hi)
#define LOFRACTION(X)	(((_dval *)&(X))->fparts.lo)
#define QNANBIT(X)	(((_dval *)&(X))->nparts.qnan_bit)
#define HIWORD(X)	(((_dval *)&(X))->fwords.hi)
#define LOWORD(X)	(((_dval *)&(X))->fwords.lo)

#define MAXEXP	0x7ff /* maximum exponent of double*/
#define ISMAXEXP(X)	((EXPONENT(X)) == MAXEXP)

/* macros used to create quiet NaNs as return values */
#define SETQNAN(X)	((((_dval *)&(X))->nparts.qnan_bit) = 0x1)
#define HIQNAN(X)	((HIWORD(X)) = 0x7ff80000)
#define LOQNAN(X)	((((_dval *)&(X))->fwords.lo) = 0x0)

/* make a NaN out of a double */
#define MKNAN(X)	(HIQNAN(X),LOQNAN(X))

/* like libc's nan.h macros */
#ifndef NANorINF
#define NANorINF(X)	(ISMAXEXP(X))
#endif

/* NANorINF must be called before this */
#ifndef INF
#define INF(X)	(!LOWORD(X) && !HIFRACTION(X))
#endif

/* 
 * If a NaN is not a quiet NaN, raise an invalid-op exception and 
 * make it into a quiet NaN.
 */

#define SigNAN(X) if (!QNANBIT(X)) { 	double q1=0.0; 		\
					double q2=0.0; 		\
								\
					q1 /= q2; 		\
 					SETQNAN(X); 		\
				    } 				
   

/* macros used to extract parts of single precision values */
#define	FSIGNBIT(X)	(((_fval *)&(X))->fparts.sign)
#define FEXPONENT(X)	(((_fval *)&(X))->fparts.exp)
#define FFRACTION(X)	(((_fval *)&(X))->fparts.fract)
#define FWORD(X)	(((_fval *)&(X))->fword)
#define FQNANBIT(X)	(((_fval *)&(X))->nparts.qnan_bit)
#define MAXEXPF	255 /* maximum exponent of single*/
#define FISMAXEXP(X)	((FEXPONENT(X)) == MAXEXPF)

/* macros used to create quiet NaNs as return values */
#define FSETQNAN(X)	((FQNANBIT(X)) = 0x1)
#define FMKNAN(X)	((FWORD(X)) = 0x7fc00000)

/* like libc's nan.h macros */
#ifndef FNANorINF
#define FNANorINF(X)	(FISMAXEXP(X))
#endif

/* FNANorINF must be called before this */
#ifndef FINF
#define FINF(X)	(!FFRACTION(X))
#endif

/* 
 * If a NaN is not a quiet NaN, raise an invalid-op exception and 
 * make it into a quiet NaN.
 */

#define FSigNAN(X) if (!FQNANBIT(X)) { 	float q1=0.0; 		\
					float q2=0.0; 		\
								\
					q1 /= q2; 		\
 					FSETQNAN(X); 		\
				    } 				
#else
	/* non IEEE */
#define MKNAN(X)	(X = 0.0)
#define FMKNAN(X)	(X = 0.0)

#endif  /*IEEE*/

#ifdef __STDC__
extern float	_float_domain(float, float, float, const char *, int);
extern double	_domain_err(double, double, double, char *, int);
#else
extern double	_domain_err();
#endif

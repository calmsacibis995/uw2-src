/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/fabs.c	1.5"
/*LINTLIBRARY*/
/*
 *	fabs returns the absolute value of its double-precision argument.
 *
 *	If the argument is a NaN, returns error EDOM and the argument.
 *	If the argument is also a signalling NaN, and invalid op exception
 *	is raised.
 */

#ifdef __STDC__
	#pragma weak copysign = _copysign
#endif

#include "synonyms.h"
#include <values.h>
#include "fpparts.h"
#include <errno.h>

double
fabs(x)
double x;
{

#if _IEEE
	if (NANorINF(x) && !INF(x)) 	/* x is a NaN */
		return _domain_err(x,0.0,x,"fabs",4);
	else
		SIGNBIT(x) = 0;

	return x;
#else
	return (x < 0 ? -x : x);
#endif
}

/* COPYSIGN(X,Y)
 * Return x with the sign of y  - no exceptions are raised
 */


double copysign(x,y)
double	x, y;
{
#if _IEEE
	SIGNBIT(x) = SIGNBIT(y);
	return x ;
#else
	if (y >= 0.0)
		return(x >= 0.0 ? x : -x);
	else 
		return(x >= 0.0 ? -x : x);
#endif
}

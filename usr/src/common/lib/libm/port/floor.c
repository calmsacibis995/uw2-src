/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/floor.c	1.2"
/*LINTLIBRARY*/
/*
 *	floor(x) returns the largest integer (as a double-precision number)
 *	not greater than x.
 *	ceil(x) returns the smallest integer not less than x.
 *
 *	If x is a NaN, EDOM error and value x is returned.  If x is a
 *	signalling NaN, an invalid op exception is also raised.
 */

#include <values.h>
#include <math.h>
#include "fpparts.h"
#include <errno.h>

double
floor(x) 
double x;
{
	double y; /* can't be in register because of modf() below */

	/* Have to check for NaN else comparison below would cause invalid op */
	if (NANorINF(x) && !INF(x))
		return _domain_err(x,0.0,x,"floor",5);

	return (modf(x, &y) < 0 ? y - 1 : y);
}

double
ceil(x)
double x;
{
	double y; /* can't be in register because of modf() below */

	/* Have to check for NaN else comparison below would cause invalid op */
	if (NANorINF(x) && !INF(x)) 
		return _domain_err(x,0.0,x,"ceil",4);

	return (modf(x, &y) > 0 ? y + 1 : y);
}

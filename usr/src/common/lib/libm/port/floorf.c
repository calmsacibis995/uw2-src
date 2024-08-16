/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/floorf.c	1.3"
/*LINTLIBRARY*/
/*
 *	Single Precision Routine - float args and float return.
 *	floor(x) returns the largest integer (as a single-precision number)
 *	not greater than x.
 *	ceil(x) returns the smallest integer not less than x.
 *	Returns EDOM error and
 *	value NaN if argument is a NaN
 *
 *	An invalid op exception is raised if argument
 *	is a signalling NaN.
 */

#include <values.h>
#include "fpparts.h"
#include <math.h>

float
floorf(float x) 
{
	float y;

	if (FNANorINF(x) && !FINF(x))
		return _float_domain(x,0.0,x,"floorf",6);
	return (modff(x, &y) < 0 ? y - 1 : y);
}

float
ceilf(float x)
{
	float y;

	if (FNANorINF(x) && !FINF(x))
		return _float_domain(x,0.0,x,"ceilf",5);
	return (modff(x, &y) > 0 ? y + 1 : y);
}

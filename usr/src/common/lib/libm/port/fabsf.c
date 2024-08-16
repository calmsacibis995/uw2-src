/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/fabsf.c	1.2"
/*LINTLIBRARY*/
/*
 *	Single Precision Routine - float args and float return.
 *	fabs returns the absolute value of its single-precision argument.
 *	Returns EDOM error and
 *	value NaN if argument is a NaN
 *
 *	An invalid op exception is raised if argument
 *	is a signalling NaN.
 */
#include <values.h>
#include "fpparts.h"

float
fabsf(float x)
{
#ifdef _IEEE
	if (FNANorINF(x) && !FINF(x)) 	/* x is a NaN */
		return _float_domain(x,0.0F,x,"fabsf",5);
	FSIGNBIT(x) = 0;
	return x;
#else
	return (x < 0 ? -x : x);
#endif
}

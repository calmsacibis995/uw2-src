/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/frexp.c	1.9.1.6"
/*LINTLIBRARY*/
/*
 * frexp(value, eptr)
 * returns a double x such that x = 0 or 0.5 <= |x| < 1.0
 * and stores an integer n such that value = x * 2 ** n
 * indirectly through eptr.
 *
 * If value is a NaN returns error EDOM and value NaN.  If value is also 
 * a signalling NaN, raise an invalid op exception.
 *
 */
#include <sys/types.h>
#include "synonyms.h"
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <values.h>
#include "fpparts.h"

double
frexp(value, eptr)
double value; 
register int *eptr;
{
	register double absvalue;

	if (NANorINF(value)){
		if (!INF(value)){	/* value is NaN */

			/* Cause exception if not quiet */
			SigNAN(value);

			errno=EDOM;
		}
			
		return value;
	}

	*eptr = 0;
	if (value == 0.0) /* nothing to do for zero */
		return (value);
	absvalue = (value > 0.0) ? value : -value;
	for ( ; absvalue >= 1.0; absvalue *= 0.5)
		++*eptr;
	for ( ; absvalue < 0.5; absvalue += absvalue)
		--*eptr;
	return (value > 0.0 ? absvalue : -absvalue);
}

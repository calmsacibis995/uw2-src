/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/atanh.c	1.7"
/*LINTLIBRARY*/
/* 
 */

/* ATANH(X)
 *	log1p(x) 	...return log(1+x)
 * Method :
 *	Return 
 *                          1              2x                          x
 *		atanh(x) = --- * log(1 + -------) = 0.5 * log1p(2 * --------)
 *                          2             1 - x                      1 - x
 *
 *	In -Xt	if |x| == 1.0, it's a singularity error 
 *	Returns EDOM error and
 *	value NaN if argument is a NaN or |argument| > 1.0.
 *
 *	An invalid op exception is raised if argument
 *	is a signalling NaN or |argument| > 1.0
 */

#include "synonyms.h"
#include <math.h>
#include <errno.h>
#include <values.h>
#include "fpparts.h"

double atanh(x)
double x;
{
	double _log1p();
	register double y, z;

	if (NANorINF(x) && !INF(x)) {
		return (_domain_err(x, 0.0, x, "atanh", 5));
	} 
	if ((y = _ABS(x)) >= 1.0) {
		double	p;
		double	q1 = 0.0, q2 = 0.0;

		MKNAN(p);
		if (y == 1.0) {
			if (_lib_version == c_issue_4) {
				struct exception exc;
				exc.type = SING;
				exc.retval = p;
				exc.name = "atanh";
				exc.arg1 = x;
				if (!matherr(&exc)) {
					
					(void) write(2, 
				  	    "atanh: SING error\n", 19);
					errno = EDOM;
				}
				return(exc.retval);
			}
			else
				return (x == -1.0 ? -HUGE_VAL 
					: HUGE_VAL);
		}

#if _IEEE
		/* raise invalid op */
		q1 /= q2;
#endif
		return (_domain_err(x, 0.0, p, "acosh", 5));
	}
	z = ((x < 0.0) ? -0.5 : 0.5);
	y /=  (1.0 - y);
	return(z * _log1p(y + y));
}

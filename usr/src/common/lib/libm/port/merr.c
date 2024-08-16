/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libm:port/merr.c	1.2"

#include <values.h>
#include "fpparts.h"
#include <errno.h>
#include <math.h>

double
_domain_err(x,y,ret,name,len)
double x;			/* x arg */
double y;			/* y arg */
double ret;			/* return value */
char *name;
int len;
{
	struct exception exc;

	/* raise an exception if a signalling NaN */
	if (NANorINF(ret) && !INF(ret))
		SigNAN(ret);
	
	exc.retval = ret;
	if (_lib_version == c_issue_4) {
		exc.arg1 = x;
		exc.arg2 = y;
		exc.name = name;
		exc.type = DOMAIN;
		if (!matherr(&exc)) {
			(void) write(2, exc.name, len);
			(void) write(2, ": DOMAIN error\n", 15);
			errno=EDOM;
		}
	} else
		errno=EDOM;

	return (exc.retval);
}

float
_float_domain(float x, float y, float ret, const char *name, int len)
{
	struct exception exc;

	/* raise an exception if a signalling NaN */
	if (FNANorINF(ret) && !FINF(ret))
		FSigNAN(ret);
	
	if (_lib_version == c_issue_4) {
		exc.retval = (double)ret;
		exc.arg1 = (double)x;
		exc.arg2 = (double)y;
		exc.name = (char *)name;
		exc.type = DOMAIN;
		if (!matherr(&exc)) {
			(void) write(2, exc.name, len);
			(void) write(2, ": DOMAIN error\n", 15);
			errno = EDOM;
		}
		return((float)exc.retval);
	}
	errno = EDOM;
	return ret;
}

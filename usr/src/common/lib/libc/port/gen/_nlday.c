/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_nlday.c	1.2"

#include <time.h>
#include "timem.h"

long
#ifdef __STDC__
_nlday(long yr)	/* number of leap days relative to Epoch to beginning of yr */
#else
_nlday(yr)long yr;
#endif
{
	long n = -(EPOCH_YEAR / 4 - EPOCH_YEAR / 100 + EPOCH_YEAR / 400);

	yr--;
	/*
	* This pretends as if there were leap years from year 1 A.D.,
	* the first thus being year 4.  In actuality, there were no
	* leap years until 1752, which was "fixed" in September.
	*/
	if ((yr >>= 2) != 0)
	{
		n += yr;
		if ((yr /= 25) != 0)
		{
			n -= yr;
			if ((yr >>= 2) != 0)
				n += yr;
		}
	}
	return n;
}

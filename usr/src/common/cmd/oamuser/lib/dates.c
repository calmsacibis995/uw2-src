/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:lib/dates.c	1.3.10.2"
#ident  "$Header: dates.c 2.0 91/07/13 $"

#include	<time.h>

extern int putenv();

static int mask_defined = 0;

static char *dmaskpath = "DATEMSK=/etc/datemsk";

extern int _getdate_err;

/* Parse a date string and return time_t value */
time_t
p_getdate( string )
char *string;
{
	struct tm *tmptr, *getdate();
	time_t rtime;

	if ( !mask_defined ) {
		if ( putenv( dmaskpath ) != 0 )
			return( (time_t) 0 );
		mask_defined = 1;
	}
	if( !(tmptr = getdate( string )) )
		return( (time_t) 0 );
	if ( (rtime = mktime( tmptr )) < 0)
		return( (time_t) 0);
	return( rtime );
}


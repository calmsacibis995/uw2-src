/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/time_gdata.c	1.4"

#ifdef __STDC__
	#pragma weak altzone = _altzone
	#pragma weak daylight = _daylight
	#pragma weak timezone = _timezone
	#pragma weak tzname = _tzname
#endif
#include	"synonyms.h"
#include	<sys/types.h>
#include 	<time.h>

time_t	timezone = 0;
time_t	altzone = 0;
int 	daylight = 0;
char 	*tzname[] = {"GMT","   "};

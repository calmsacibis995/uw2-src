/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tzset.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <limits.h>
#include <string.h>
#include <time.h>
#include "timem.h"

#ifdef __STDC__
	#pragma weak tzset = _tzset
#endif

void
#ifdef __STDC__
tzset(void)	/* sets globals now only used by applications */
#else
tzset()
#endif
{
	static char tzstr[2][TZNAME_MAX + 1];
	struct tm tmbuf;
	struct tz_info tzbuf;

	_tz_info(&tzbuf);
	if (tzbuf.str[0][0] == '\0')
		(void)_tz_file(&tzbuf, (long)time((time_t *)0), 0);
	daylight = tzbuf.str[1][0] != '\0';
	timezone = tzbuf.off[0];
	altzone = daylight ? tzbuf.off[1] : timezone;
	tzname[0] = strcpy(tzstr[0], tzbuf.str[0]);
	tzname[1] = strcpy(tzstr[1],
		daylight ? tzbuf.str[1] : (const char *)"   ");
}

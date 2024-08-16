/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/uglydate.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */

/*
 * uglydate - convert a time stamp to something barely readable
 *	      The string returned is 37 characters long.
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "ntp_fp.h"
#include "ntp_unixtime.h"
#include "lib_strbuf.h"

char *
uglydate(ts)
	l_fp *ts;
{
	char *bp;
	char *timep;
	struct tm *tm;
	u_long sec;
	u_long msec;
	int year;
	extern char *ulfptoa();

	timep = ulfptoa(ts, 6);		/* returns max 17 characters */

	LIB_GETBUF(bp);
	
	sec = ts->l_ui - JAN_1970;
	msec = ts->l_uf / 4294967;	/* fract / (2**32/1000) */
	tm = gmtime((long *)&sec);
	if (ts->l_ui == 0) {
		/*
		 * Probably not a real good thing to do.  Oh, well.
		 */
		year = 0;
		tm->tm_yday = 0;
		tm->tm_hour = 0;
		tm->tm_min = 0;
		tm->tm_sec = 0;
	} else {
		year = tm->tm_year;
		while (year >= 100)
			year -= 100;
	}

	(void) sprintf(bp, "%17s %02d:%03d:%02d:%02d:%02d.%03d",
	    timep, year, tm->tm_yday, tm->tm_hour, tm->tm_min,
	    tm->tm_sec, msec);

	return bp;
}

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Time/tm/tmfix.c	3.3" */
/******************************************************************************
*
* C++ Standard Components, Release 3.0.
*
* Copyright (c) 1991, 1992 AT&T and Unix System Laboratories, Inc.
* Copyright (c) 1988, 1989, 1990 AT&T.  All Rights Reserved.
*
* THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T and Unix System
* Laboratories, Inc.  The copyright notice above does not evidence
* any actual or intended publication of such source code.
*
******************************************************************************/

#include <tm.h>

/*
 * correct out of bounds fields in tp
 *
 * tm_wday, tm_yday and tm_isdst are not changed
 * as these can be computed from the other fields
 *
 * tp is the return value
 */

struct tm* tmfix_ATTLC(register struct tm* tp) {
	register int	n;

	if ((n = tp->tm_sec) < 0)
	{
		tp->tm_min -= (-n) / 60 + 1;
		tp->tm_sec = 60 - (-n) % 60;
	}
	else if (n > (59 + TM_MAXLEAP))
	{
		tp->tm_min += n / 60;
		tp->tm_sec %= 60;
	}
	if ((n = tp->tm_min) < 0)
	{
		tp->tm_hour -= (-n) / 60 + 1;
		tp->tm_min = 60 - (-n) % 60;
	}
	else if (n > 59)
	{
		tp->tm_hour += n / 60;
		tp->tm_min %= 60;
	}
	if ((n = tp->tm_hour) < 0)
	{
		tp->tm_mday -= (-n) / 24 + 1;
		tp->tm_hour = 24 - (-n) % 24;
	}
	else if (n > 23)
	{
		tp->tm_mday += n / 24;
		tp->tm_hour %= 24;
	}
	while (tp->tm_mday < 1)
	{
		if (--tp->tm_mon < 0)
		{
			tp->tm_mon = 11; /* set month to December */
			tp->tm_year--;
		}
		tp->tm_mday += tm_mcnt_ATTLC[tp->tm_mon] +
			(tp->tm_mon == 1 && !(tp->tm_year % 4));
	}
	while (tp->tm_mday > (n = tm_mcnt_ATTLC[tp->tm_mon] + (tp->tm_mon == 1 && !(tp->tm_year % 4))))
	{
		tp->tm_mday -= n;
		if (++tp->tm_mon > 11)
		{
			tp->tm_mon = 0;
			tp->tm_year++;
		}
	}
	return(tp);
}

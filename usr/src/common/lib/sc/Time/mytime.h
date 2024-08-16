/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Time/mytime.h	3.2" */
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

#ifndef MYTIME_H
#define MYTIME_H

//  First, take everything from the standard time.h:

#include <time.h>

extern long timezone_ATTLC;
#define MAXTZNAME 3
extern long altzone_ATTLC;
extern int daylight_ATTLC;
extern char tzname_ATTLC[][MAXTZNAME+1];

//  The following variables and functions are declared as
//  static in time_comm.c.  We need them to be extern:

extern long start_dst_ATTLC; /* Start date of alternate time zone */
extern long end_dst_ATTLC;   /* End date of alternate time zone */
extern tm *localtime_ATTLC(time_t*);
extern void getusa_ATTLC(long*,long*,tm*);
extern tm *gmtime_ATTLC(time_t*);
extern void tzset_ATTLC();

#endif

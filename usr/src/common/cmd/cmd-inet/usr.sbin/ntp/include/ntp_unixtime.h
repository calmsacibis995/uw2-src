/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/include/ntp_unixtime.h	1.2"
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
 * ntp_unixtime.h - contains constants and macros for converting between
 *		    NTP time stamps (l_fp) and Unix times (struct timeval)
 */

/*
 * Time of day conversion constant.  Ntp's time scale starts in 1900,
 * Unix in 1970.
 */
#define	JAN_1970	2208988800	/* 1970 - 1900 in seconds */

/*
 * These constants are used to round the time stamps computed from
 * a struct timeval to the microsecond (more or less).  This keeps
 * things neat.
 */
#define	TS_MASK		0xfffff000	/* mask to usec, for time stamps */
#define	TS_ROUNDBIT	0x00000800	/* round at this bit */


/*
 * Convert usec to a time stamp fraction.  If you use this the program
 * must include the following declarations:
 *
 * extern u_long ustotslo[];
 * extern u_long ustotsmid[];
 * extern u_long ustotshi[];
 */
#define	TVUTOTSF(tvu, tsf) \
	(tsf) = ustotslo[(tvu) & 0xff] \
	    + ustotsmid[((tvu) >> 8) & 0xff] \
	    + ustotshi[((tvu) >> 16) & 0xf]

/*
 * Convert a struct timeval to a time stamp.
 */
#define TVTOTS(tv, ts) \
	do { \
		(ts)->l_ui = (unsigned long)(tv)->tv_sec; \
		TVUTOTSF((tv)->tv_usec, (ts)->l_uf); \
	} while(0)

/*
 * TV_SHIFT is used to turn the table result into a usec value.  To round,
 * add in TV_ROUNDBIT before shifting
 */
#define	TV_SHIFT	3
#define	TV_ROUNDBIT	0x4


/*
 * Convert a time stamp fraction to microseconds.  The time stamp
 * fraction is assumed to be unsigned.  To use this in a program, declare:
 *
 * extern long tstouslo[];
 * extern long tstousmid[];
 * extern long tstoushi[];
 */
#define	TSFTOTVU(tsf, tvu) \
	(tvu) = (tstoushi[((tsf) >> 24) & 0xff] \
	    + tstousmid[((tsf) >> 16) & 0xff] \
	    + tstouslo[((tsf) >> 9) & 0x7f] \
	    + TV_ROUNDBIT) >> TV_SHIFT
/*
 * Convert a time stamp to a struct timeval.  The time stamp
 * has to be positive.
 */
#define	TSTOTV(ts, tv) \
	do { \
		(tv)->tv_sec = (ts)->l_ui; \
		TSFTOTVU((ts)->l_uf, (tv)->tv_usec); \
		if ((tv)->tv_usec == 1000000) { \
			(tv)->tv_sec++; \
			(tv)->tv_usec = 0; \
		} \
	} while (0)

/*
 * Convert milliseconds to a time stamp fraction.  This shouldn't be
 * here, but it is convenient since the guys who use the definition will
 * often be including this file anyway.
 */
#define	MSUTOTSF(msu, tsf) \
	(tsf) = msutotsfhi[((msu) >> 5) & 0x1f] + msutotsflo[(msu) & 0x1f]

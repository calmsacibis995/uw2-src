/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/clocktime.c	1.2"
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
 * clocktime - compute the NTP date from a day of year, hour, minute
 *	       and second.
 */
#include <sys/types.h>
#include <sys/time.h>

#include "ntp_fp.h"
#include "ntp_unixtime.h"


/*
 * Hacks to avoid excercising the multiplier.  I have no pride.
 */
#define	MULBY10(x)	(((x)<<3) + ((x)<<1))
#define	MULBY60(x)	(((x)<<6) - ((x)<<2))	/* watch overflow */
#define	MULBY24(x)	(((x)<<4) + ((x)<<3))

/*
 * Two days, in seconds.
 */
#define	TWODAYS		(2*24*60*60)

/*
 * We demand that the time be within CLOSETIME seconds of the receive
 * time stamp.  This is about 4 hours, which hopefully should be
 * wide enough to collect most data, while close enough to keep things
 * from getting confused.
 */
#define	CLOSETIME	(4*60*60)


int
clocktime(yday, hour, minute, second, tzoff, rec_ui, yearstart, ts_ui)
	int yday;
	int hour;
	int minute;
	int second;
	int tzoff;
	u_long rec_ui;
	u_long *yearstart;
	u_long *ts_ui;
{
	register long tmp;
	register u_long date;
	register u_long yst;
	extern u_long calyearstart();

	/*
	 * Compute the offset into the year in seconds.  Note that
	 * this could come out to be a negative number.
	 */
	tmp = (long)(MULBY24((yday-1)) + hour + tzoff);
	tmp = MULBY60(tmp) + (long)minute;
	tmp = MULBY60(tmp) + (long)second;

	/*
	 * Initialize yearstart, if necessary.
	 */
	yst = *yearstart;
	if (yst == 0) {
		yst = calyearstart(rec_ui);
		*yearstart = yst;
	}

	/*
	 * Now the fun begins.  We demand that the received clock time
	 * be within CLOSETIME of the receive timestamp, but
	 * there is uncertainty about the year the timestamp is in.
	 * Use the current year start for the first check, this should
	 * work most of the time.
	 */
	date = (u_long)(tmp + (long)yst);
	if (date < (rec_ui + CLOSETIME) &&
	    date > (rec_ui - CLOSETIME)) {
		*ts_ui = date;
		return 1;
	}

	/*
	 * Trouble.  Next check is to see if the year rolled over and, if
	 * so, try again with the new year's start.
	 */
	yst = calyearstart(rec_ui);
	if (yst != *yearstart) {
		date = (u_long)((long)yst + tmp);
		*ts_ui = date;
		if (date < (rec_ui + CLOSETIME) &&
		    date > (rec_ui - CLOSETIME)) {
			*yearstart = yst;
			return 1;
		}
	}

	/*
	 * Here we know the year start matches the current system
	 * time.  One remaining possibility is that the time code
	 * is in the year previous to that of the system time.  This
	 * is only worth checking if the receive timestamp is less
	 * than a couple of days into the new year.
	 */
	if ((rec_ui - yst) < TWODAYS) {
		yst = calyearstart(yst - TWODAYS);
		if (yst != *yearstart) {
			date = (u_long)(tmp + (long)yst);
			if (date < (rec_ui + CLOSETIME) &&
			    date > (rec_ui - CLOSETIME)) {
				*yearstart = yst;
				*ts_ui = date;
				return 1;
			}
		}
	}

	/*
	 * One last possibility is that the time stamp is in the year
	 * following the year the system is in.  Try this one before
	 * giving up.
	 */
	yst = calyearstart(rec_ui + TWODAYS);
	if (yst != *yearstart) {
		date = (u_long)((long)yst + tmp);
		if (date < (rec_ui + CLOSETIME) &&
		    date > (rec_ui - CLOSETIME)) {
			*yearstart = yst;
			*ts_ui = date;
			return 1;
		}
	}

	/*
	 * Give it up.
	 */
	return 0;
}

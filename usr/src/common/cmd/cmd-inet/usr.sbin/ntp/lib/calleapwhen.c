/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/calleapwhen.c	1.2"
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
 * calleapwhen - determine the number of seconds to the next possible
 *		 leap occurance and the last one.
 */
#include <sys/types.h>

#include "ntp_calendar.h"

/*
 * calleaptab - leaps occur at the end of December and June
 */
long calleaptab[10] = {
	-(JAN+FEBLEAP)*SECSPERDAY,	/* leap previous to cycle */
	(MAR+APR+MAY+JUN)*SECSPERDAY,	/* end of June */
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC)*SECSPERDAY, /* end of Dec */
	(MAR+APR+MAY+JUN)*SECSPERDAY + SECSPERYEAR,
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC)*SECSPERDAY + SECSPERYEAR,
	(MAR+APR+MAY+JUN)*SECSPERDAY + 2*SECSPERYEAR,
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC)*SECSPERDAY + 2*SECSPERYEAR,
	(MAR+APR+MAY+JUN)*SECSPERDAY + 3*SECSPERYEAR,
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC)*SECSPERDAY + 3*SECSPERYEAR,
	(MAR+APR+MAY+JUN+JUL+AUG+SEP+OCT+NOV+DEC+JAN+FEBLEAP+MAR+APR+MAY+JUN)
	    *SECSPERDAY + 3*SECSPERYEAR,	/* next after current cycle */
};

void
calleapwhen(ntpdate, leaplast, leapnext)
	u_long ntpdate;
	u_long *leaplast;
	u_long *leapnext;
{
	register u_long dateincycle;
	register int i;

	/*
	 * Find the offset from the start of the cycle
	 */
	dateincycle = ntpdate;
	if (dateincycle >= MAR1988)
		dateincycle -= MAR1988;
	else
		dateincycle -= MAR1900;

	while (dateincycle >= SECSPERCYCLE)
		dateincycle -= SECSPERCYCLE;

	/*
	 * Find where we are with respect to the leap events.
	 */
	for (i = 1; i < 9; i++)
		if (dateincycle < (u_long)calleaptab[i])
			break;
	
	/*
	 * i points at the next leap.  Compute the last and the next.
	 */
	*leaplast = (u_long)((long)dateincycle - calleaptab[i-1]);
	*leapnext = (u_long)(calleaptab[i] - (long)dateincycle);
}

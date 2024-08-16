/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/buftvtots.c	1.2"
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
 * buftvtots - pull a Unix-format (struct timeval) time stamp out of
 *	       an octet stream and convert it to a l_fp time stamp.
 *	       This is useful when using the clock line discipline.
 */
#include <sys/types.h>
#include <sys/time.h>

#include "ntp_fp.h"
#include "ntp_unixtime.h"

/*
 * Conversion tables
 */
extern u_long ustotslo[];
extern u_long ustotsmid[];
extern u_long ustotshi[];

int
buftvtots(bufp, ts)
	char *bufp;
	l_fp *ts;
{
	register u_char *bp;
	register u_long sec;
	register u_long usec;

	bp = (u_char *)bufp;

	sec = (u_long)*bp++ & 0xff;
	sec <<= 8;
	sec += (u_long)*bp++ & 0xff;
	sec <<= 8;
	sec += (u_long)*bp++ & 0xff;
	sec <<= 8;
	sec += (u_long)*bp++ & 0xff;

	usec = (u_long)*bp++ & 0xff;
	usec <<= 8;
	usec += (u_long)*bp++ & 0xff;
	usec <<= 8;
	usec += (u_long)*bp++ & 0xff;
	usec <<= 8;
	usec += (u_long)*bp & 0xff;

	if (usec > 999999)
		return 0;

	ts->l_ui = sec + (u_long)JAN_1970;
	TVUTOTSF(usec, ts->l_uf);
	return 1;
}

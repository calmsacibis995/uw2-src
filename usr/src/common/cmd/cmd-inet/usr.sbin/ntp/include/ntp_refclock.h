/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/include/ntp_refclock.h	1.2"
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
 * ntp_refclock.h - definitions for reference clock support
 */

/*
 * Macros to determine the clock type and unit numbers from a
 * 127.127.t.u address.
 */
#define	REFCLOCKTYPE(srcadr)	((SRCADR(srcadr) >> 8) & 0xff)
#define REFCLOCKUNIT(srcadr)	(SRCADR(srcadr) & 0xff)

/*
 * Struct refclock provides the interface between the reference
 * clock support and particular clock drivers.  There are entries
 * to open and close a unit, optional values to specify the
 * timer interval for calls to the transmit procedure and to
 * specify a polling routine to be called when the transmit
 * procedure executes.  There is an entry which is called when
 * the transmit routine is about to shift zeroes into the
 * filter register, and entries for stuffing fudge factors into
 * the driver and getting statistics from it.
 */
struct refclock {
	int (*clock_start)();		/* start a clock unit */
	void (*clock_shutdown)();	/* shut a clock down */
	void (*clock_poll)();		/* called from the xmit routine */
	void (*clock_leap)();		/* inform driver a leap has occured */
	void (*clock_control)();	/* set fudge values, return stats */
	void (*clock_init)();		/* initialize driver data at startup */
	void (*clock_buginfo)();	/* get clock dependent bug info */
	u_long clock_xmitinterval;	/* timer setting for xmit routine */
	u_long clock_flags;		/* flag values */
};

/*
 * Definitions for default values
 */
#define	noentry		0
#define	STDPOLL		(1<<NTP_MINPOLL)
#define	NOPOLL		0

/*
 * Definitions for flags
 */
#define	NOFLAGS			0
#define	REF_FLAG_BCLIENT	0x1	/* clock prefers to run as a bclient */

/*
 * Flag values
 */
#define	CLK_HAVETIME1	0x1
#define	CLK_HAVETIME2	0x2
#define	CLK_HAVEVAL1	0x4
#define	CLK_HAVEVAL2	0x8

#define	CLK_FLAG1	0x1
#define	CLK_FLAG2	0x2
#define	CLK_FLAG3	0x4
#define	CLK_FLAG4	0x8

#define	CLK_HAVEFLAG1	0x10
#define	CLK_HAVEFLAG2	0x20
#define	CLK_HAVEFLAG3	0x40
#define	CLK_HAVEFLAG4	0x80

/*
 * Structure for returning clock status
 */
struct refclockstat {
	u_char type;
	u_char flags;
	u_char haveflags;
	u_char lencode;
	char *lastcode;
	u_long polls;
	u_long noresponse;
	u_long badformat;
	u_long baddata;
	u_long timereset;
	char *clockdesc;	/* description of clock, in ASCII */
	l_fp fudgetime1;
	l_fp fudgetime2;
	long fudgeval1;
	long fudgeval2;
	u_char currentstatus;
	u_char lastevent;
	u_char unused[2];
};


/*
 * Reference clock I/O structure.  Used to provide an interface between
 * the reference clock drivers and the I/O module.
 */
struct refclockio {
	struct refclockio *next;
	void (*clock_recv)();
	caddr_t srcclock;	/* pointer to clock structure */
	int datalen;
	int fd;
	u_long recvcount;
};


/*
 * Sizes of things we return for debugging
 */
#define	NCLKBUGVALUES		16
#define	NCLKBUGTIMES		32

/*
 * Structure for returning debugging info
 */
struct refclockbug {
	u_char nvalues;
	u_char ntimes;
	u_short svalues;
	u_long stimes;
	u_long values[NCLKBUGVALUES];
	l_fp times[NCLKBUGTIMES];
};
